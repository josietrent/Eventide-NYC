import { join } from "@std/path";
import { ensureDir } from "@std/fs/ensure-dir";

const RW_VER = 0x0800FFFF;
const dec = new TextDecoder();
const enc = new TextEncoder();

function u32(val: number): Uint8Array {
  const b = new Uint8Array(4);
  new DataView(b.buffer).setUint32(0, val >>> 0, true);
  return b;
}

function f32s(vals: number[]): Uint8Array {
  const b = new Uint8Array(vals.length * 4);
  const dv = new DataView(b.buffer);
  for (let i = 0; i < vals.length; i++) dv.setFloat32(i * 4, vals[i], true);
  return b;
}

function concat(parts: Uint8Array[]): Uint8Array {
  const total = parts.reduce((acc, p) => acc + p.length, 0);
  const out = new Uint8Array(total);
  let off = 0;
  for (const p of parts) {
    out.set(p, off);
    off += p.length;
  }
  return out;
}

function rwChunk(type: number, payload: Uint8Array, ver = RW_VER): Uint8Array {
  const hdr = new Uint8Array(12);
  const dv = new DataView(hdr.buffer);
  dv.setUint32(0, type >>> 0, true);
  dv.setUint32(4, payload.length >>> 0, true);
  dv.setUint32(8, ver >>> 0, true);
  return concat([hdr, payload]);
}

async function readGta3ImgEntry(
  gta3Dir: string,
  entryName: string,
): Promise<Uint8Array> {
  const dirBytes = await Deno.readFile(join(gta3Dir, "models", "gta3.dir"));
  const dirDv = new DataView(dirBytes.buffer);
  const target = entryName.toLowerCase();
  for (let i = 0; i < dirBytes.length; i += 32) {
    const rawName = dirBytes.subarray(i + 8, i + 32);
    const z = rawName.indexOf(0);
    const name = dec.decode(z >= 0 ? rawName.subarray(0, z) : rawName).toLowerCase();
    if (name === target) {
      const off = dirDv.getUint32(i, true) * 2048;
      const size = dirDv.getUint32(i + 4, true) * 2048;
      const imgFile = await Deno.open(join(gta3Dir, "models", "gta3.img"), {
        read: true,
      });
      const buf = new Uint8Array(size);
      await imgFile.seek(off, Deno.SeekMode.Start);
      let readTotal = 0;
      while (readTotal < size) {
        const n = await imgFile.read(buf.subarray(readTotal));
        if (n === null) break;
        readTotal += n;
      }
      imgFile.close();
      return buf;
    }
  }
  throw new Error(`Missing ${entryName} in gta3.dir`);
}

function buildAntennaDff(
  indRadioDff: Uint8Array,
  frameName: string,
): Uint8Array {
  const dv = new DataView(
    indRadioDff.buffer,
    indRadioDff.byteOffset,
    indRadioDff.byteLength,
  );

  // Locate Geometry (0x0F) inside GeometryList (0x1A)
  let p = 12;
  let geomStart = -1;
  let geomEnd = -1;
  while (p + 12 <= indRadioDff.length) {
    const t = dv.getUint32(p, true);
    const s = dv.getUint32(p + 4, true);
    if (t === 0x1a) {
      const gp = p + 12 + 16; // after GeometryList Struct (count)
      geomStart = gp + 12;
      geomEnd = geomStart + dv.getUint32(gp + 4, true);
      break;
    }
    p += 12 + s;
  }
  if (geomStart < 0) throw new Error("Geometry not found in ind_newradio.dff");

  let flags = 0;
  let nTri = 0;
  let nVert = 0;
  const prelit: number[] = [];
  const uvs: [number, number][] = [];
  const tris: { a: number; b: number; c: number; mat: number }[] = [];
  const verts: [number, number, number][] = [];
  const matChunks: Uint8Array[] = [];

  let gp = geomStart;
  while (gp + 12 <= geomEnd) {
    const gt = dv.getUint32(gp, true);
    const gs = dv.getUint32(gp + 4, true);
    const gb = gp + 12;
    const ge = gb + gs;
    if (gt === 0x01) {
      flags = dv.getUint32(gb, true);
      nTri = dv.getUint32(gb + 4, true);
      nVert = dv.getUint32(gb + 8, true);
      let q = gb + 28;
      if (flags & 0x08) {
        for (let i = 0; i < nVert; i++) {
          prelit.push(dv.getUint32(q, true));
          q += 4;
        }
      }
      if (flags & (0x04 | 0x80)) {
        for (let i = 0; i < nVert; i++) {
          uvs.push([dv.getFloat32(q, true), dv.getFloat32(q + 4, true)]);
          q += 8;
        }
      }
      for (let i = 0; i < nTri; i++) {
        tris.push({
          b: dv.getUint16(q, true),
          a: dv.getUint16(q + 2, true),
          mat: dv.getUint16(q + 4, true),
          c: dv.getUint16(q + 6, true),
        });
        q += 8;
      }
      q += 16 + 8; // bounding sphere + hasPos/hasNorm
      for (let i = 0; i < nVert; i++) {
        verts.push([
          dv.getFloat32(q, true),
          dv.getFloat32(q + 4, true),
          dv.getFloat32(q + 8, true),
        ]);
        q += 12;
      }
    } else if (gt === 0x08) {
      let mp = gb + 12 + dv.getUint32(gb + 4, true);
      while (mp + 12 <= ge) {
        const mt = dv.getUint32(mp, true);
        const ms = dv.getUint32(mp + 4, true);
        if (mt === 0x07) {
          matChunks.push(indRadioDff.slice(mp, mp + 12 + ms));
        }
        mp += 12 + ms;
      }
    }
    gp = ge;
  }

  // Extract mat 6 (central mast) near (8.66, 0.34) and mat 10 (lattice pyramid)
  const ox = 8.66;
  const oy = 0.34;
  const oz = 1.71;

  const pickedTris = tris.filter((t) => {
    if (t.mat === 10) return true;
    if (t.mat === 6) {
      return [t.a, t.b, t.c].some((i) => {
        const v = verts[i];
        return v[0] > 3 && v[0] < 14 && v[1] > -5 && v[1] < 6 && v[2] >= 0;
      });
    }
    return false;
  });

  if (pickedTris.length !== 12) {
    throw new Error(`Expected 12 antenna tris, found ${pickedTris.length}`);
  }

  // Remap vertices cleanly preserving exact UVs & prelit vertex colors
  const newVerts: [number, number, number][] = [];
  const newUvs: [number, number][] = [];
  const newPrelit: number[] = [];
  const vertRemap = new Map<number, number>();
  const newTris: { a: number; b: number; c: number; mat: number }[] = [];

  for (const t of pickedTris) {
    const newMat = t.mat === 6 ? 0 : 1;
    const mapIdx = (oldIdx: number) => {
      const existing = vertRemap.get(oldIdx);
      if (existing !== undefined) return existing;
      const v = verts[oldIdx];
      const nx = v[0] - ox;
      const ny = v[1] - oy;
      let nz = v[2] - oz;
      if (nz < 0) nz = 0.0; // Clamp lower end of central mast flush with base Z=0
      const idx = newVerts.length;
      newVerts.push([nx, ny, nz]);
      newUvs.push(uvs[oldIdx]);
      newPrelit.push(prelit[oldIdx]);
      vertRemap.set(oldIdx, idx);
      return idx;
    };
    newTris.push({
      a: mapIdx(t.a),
      b: mapIdx(t.b),
      c: mapIdx(t.c),
      mat: newMat,
    });
  }

  // Build Geometry Struct (0x01)
  const outVertCount = newVerts.length;
  const outTriCount = newTris.length;
  const geomStructSize =
    28 +
    outVertCount * 4 +
    outVertCount * 8 +
    outTriCount * 8 +
    16 +
    8 +
    outVertCount * 12;
  const geomStruct = new Uint8Array(geomStructSize);
  const gdv = new DataView(geomStruct.buffer);
  gdv.setUint32(0, flags, true); // 0x1002c
  gdv.setUint32(4, outTriCount, true);
  gdv.setUint32(8, outVertCount, true);
  gdv.setUint32(12, 1, true); // numMorphTargets
  gdv.setFloat32(16, 1.0, true);
  gdv.setFloat32(20, 1.0, true);
  gdv.setFloat32(24, 1.0, true);
  let q = 28;
  for (let i = 0; i < outVertCount; i++) {
    gdv.setUint32(q, newPrelit[i], true);
    q += 4;
  }
  for (let i = 0; i < outVertCount; i++) {
    gdv.setFloat32(q, newUvs[i][0], true);
    gdv.setFloat32(q + 4, newUvs[i][1], true);
    q += 8;
  }
  for (const t of newTris) {
    gdv.setUint16(q, t.b, true);
    gdv.setUint16(q + 2, t.a, true);
    gdv.setUint16(q + 4, t.mat, true);
    gdv.setUint16(q + 6, t.c, true);
    q += 8;
  }
  // Bounding sphere: center=(0,0,20), radius=20.44505
  gdv.setFloat32(q, 0.0, true);
  gdv.setFloat32(q + 4, 0.0, true);
  gdv.setFloat32(q + 8, 20.0, true);
  gdv.setFloat32(q + 12, Math.hypot(3, 3, 20), true);
  gdv.setUint32(q + 16, 1, true); // hasPositions
  gdv.setUint32(q + 20, 0, true); // hasNormals
  q += 24;
  for (let i = 0; i < outVertCount; i++) {
    gdv.setFloat32(q, newVerts[i][0], true);
    gdv.setFloat32(q + 4, newVerts[i][1], true);
    gdv.setFloat32(q + 8, newVerts[i][2], true);
    q += 12;
  }

  // MaterialList (0x08): mat 6 & mat 10
  const matListStruct = new Uint8Array(12);
  const mldv = new DataView(matListStruct.buffer);
  mldv.setUint32(0, 2, true);
  mldv.setInt32(4, -1, true);
  mldv.setInt32(8, -1, true);
  const matListChunk = rwChunk(
    0x08,
    concat([rwChunk(0x01, matListStruct), matChunks[6], matChunks[10]]),
  );

  // BinMeshPLG (0x50e)
  const mat0Indices: number[] = [];
  const mat1Indices: number[] = [];
  for (const t of newTris) {
    const arr = t.mat === 0 ? mat0Indices : mat1Indices;
    arr.push(t.a, t.b, t.c);
  }
  const binMeshBytes = new Uint8Array(
    12 + (8 + mat0Indices.length * 4) + (8 + mat1Indices.length * 4),
  );
  const bdv = new DataView(binMeshBytes.buffer);
  bdv.setUint32(0, 0, true); // tri list
  bdv.setUint32(4, 2, true); // numSplits
  bdv.setUint32(8, mat0Indices.length + mat1Indices.length, true);
  let bo = 12;
  for (const [matIdx, indices] of [
    [0, mat0Indices],
    [1, mat1Indices],
  ] as const) {
    bdv.setUint32(bo, indices.length, true);
    bdv.setUint32(bo + 4, matIdx, true);
    bo += 8;
    for (const idx of indices) {
      bdv.setUint32(bo, idx, true);
      bo += 4;
    }
  }

  const geomExtChunk = rwChunk(
    0x03,
    concat([
      rwChunk(0x50e, binMeshBytes),
      rwChunk(0x105, u32(0)),
    ]),
  );

  const geometryChunk = rwChunk(
    0x0f,
    concat([rwChunk(0x01, geomStruct), matListChunk, geomExtChunk]),
  );

  const geomListChunk = rwChunk(
    0x1a,
    concat([rwChunk(0x01, u32(1)), geometryChunk]),
  );

  // FrameList (0x0E)
  const frameStruct = concat([
    u32(1),
    f32s([1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0]),
    u32(0xffffffff),
    u32(0),
  ]);
  const frameExt = rwChunk(
    0x03,
    rwChunk(0x0253f2fe, enc.encode(frameName)),
  );
  const frameListChunk = rwChunk(
    0x0e,
    concat([rwChunk(0x01, frameStruct), frameExt]),
  );

  // Atomic (0x14)
  const atomicStruct = concat([u32(0), u32(0), u32(5), u32(0)]);
  const atomicExt = rwChunk(0x03, rwChunk(0x120, u32(0)));
  const atomicChunk = rwChunk(
    0x14,
    concat([rwChunk(0x01, atomicStruct), atomicExt]),
  );

  // Clump (0x10)
  return rwChunk(
    0x10,
    concat([
      rwChunk(0x01, u32(1)),
      frameListChunk,
      geomListChunk,
      atomicChunk,
      rwChunk(0x03, new Uint8Array(0)),
    ]),
  );
}

function buildAntennaTxd(indRadioTxd: Uint8Array): Uint8Array {
  const dv = new DataView(
    indRadioTxd.buffer,
    indRadioTxd.byteOffset,
    indRadioTxd.byteLength,
  );
  const needed = new Set(["alumox64c", "alumox64b"]);
  const chunks: Uint8Array[] = [];

  let p = 12;
  p += 12 + dv.getUint32(p + 4, true); // skip TexDict Struct
  while (p + 12 <= indRadioTxd.length) {
    const t = dv.getUint32(p, true);
    const s = dv.getUint32(p + 4, true);
    if (t === 0x15) {
      const structOff = p + 24;
      const n1 = dec
        .decode(indRadioTxd.subarray(structOff + 8, structOff + 40))
        .replace(/\0.*/, "")
        .toLowerCase();
      if (needed.has(n1)) {
        chunks.push(indRadioTxd.slice(p, p + 12 + s));
      }
    }
    p += 12 + s;
  }
  if (chunks.length !== 2) {
    throw new Error(`Expected 2 TXD textures, got ${chunks.length}`);
  }

  const hdrStruct = new Uint8Array(4);
  new DataView(hdrStruct.buffer).setUint16(0, 2, true);
  return rwChunk(
    0x16,
    concat([
      rwChunk(0x01, hdrStruct),
      ...chunks,
      rwChunk(0x03, new Uint8Array(0)),
    ]),
  );
}

function buildAntennaColEntry(modelName: string, modelId: number): Uint8Array {
  // COL v1 entry:
  // 22B name + 2B modelId + 40B bounds + 4B nSpheres + 4B unknown + 4B nBoxes + 28B*1 + 4B nVerts + 12B*5 + 4B nFaces + 16B*4
  const nBoxes = 1;
  const nVerts = 5;
  const nFaces = 4;
  const bodySize =
    24 + 40 + 4 + 4 + 4 + nBoxes * 28 + 4 + nVerts * 12 + 4 + nFaces * 16;
  const out = new Uint8Array(8 + bodySize);
  const dv = new DataView(out.buffer);
  out.set(enc.encode("COLL"), 0);
  dv.setUint32(4, bodySize, true);
  out.set(enc.encode(modelName).subarray(0, 22), 8);
  dv.setUint16(30, modelId, true);

  // TBounds: radius, center(3f), min(3f), max(3f)
  dv.setFloat32(32, Math.hypot(3, 3, 20), true);
  dv.setFloat32(36, 0.0, true);
  dv.setFloat32(40, 0.0, true);
  dv.setFloat32(44, 20.0, true);
  dv.setFloat32(48, -3.0, true);
  dv.setFloat32(52, -3.0, true);
  dv.setFloat32(56, 0.0, true);
  dv.setFloat32(60, 3.0, true);
  dv.setFloat32(64, 3.0, true);
  dv.setFloat32(68, 40.0, true);

  let q = 72;
  dv.setUint32(q, 0, true); // nSpheres
  dv.setUint32(q + 4, 0, true); // unknown
  dv.setUint32(q + 8, nBoxes, true);
  q += 12;

  // Central mast box [-0.25, -0.25, 0]..[0.25, 0.25, 15.36]
  const boxVals = [-0.25, -0.25, 0.0, 0.25, 0.25, 15.36];
  for (let i = 0; i < 6; i++) {
    dv.setFloat32(q + i * 4, boxVals[i], true);
  }
  out[q + 24] = 16; // metal surface
  q += 28;

  // 5 vertices of 40m pyramid
  const verts = [
    [-3.0, -3.0, 0.0],
    [3.0, -3.0, 0.0],
    [3.0, 3.0, 0.0],
    [-3.0, 3.0, 0.0],
    [0.0, 0.0, 40.0],
  ];
  dv.setUint32(q, nVerts, true);
  q += 4;
  for (const [x, y, z] of verts) {
    dv.setFloat32(q, x, true);
    dv.setFloat32(q + 4, y, true);
    dv.setFloat32(q + 8, z, true);
    q += 12;
  }

  // 4 triangle faces
  const faces = [
    [0, 1, 4],
    [1, 2, 4],
    [2, 3, 4],
    [3, 0, 4],
  ];
  dv.setUint32(q, nFaces, true);
  q += 4;
  for (const [a, b, c] of faces) {
    dv.setUint32(q, a, true);
    dv.setUint32(q + 4, b, true);
    dv.setUint32(q + 8, c, true);
    out[q + 12] = 16; // metal surface
    q += 16;
  }
  return out;
}

const repoDir = Deno.cwd();
const envText = await Deno.readTextFile(join(repoDir, ".env"));
const gta3Match = envText.match(/^GTA3=(.+)$/m);
if (!gta3Match) throw new Error("GTA3 missing from .env");
const gta3Dir = gta3Match[1].trim().replace(/^["']|["']$/g, "");

const indRadioDff = await readGta3ImgEntry(gta3Dir, "ind_newradio.dff");
const indRadioTxd = await readGta3ImgEntry(gta3Dir, "indradio.txd");

const modelsImgDir = join(repoDir, "data", "models", "gta3.img");
await ensureDir(modelsImgDir);
await Deno.writeFile(
  join(modelsImgDir, "com_antenna.dff"),
  buildAntennaDff(indRadioDff, "com_antenna"),
);
await Deno.writeFile(
  join(modelsImgDir, "LOD_antenna.dff"),
  buildAntennaDff(indRadioDff, "LOD_antenna"),
);
await Deno.writeFile(
  join(modelsImgDir, "com_antenna.txd"),
  buildAntennaTxd(indRadioTxd),
);

const comntopOutDir = join(repoDir, "data", "maps", "comntop");
await ensureDir(comntopOutDir);

const vanillaCol = await Deno.readFile(
  join(gta3Dir, "data", "maps", "comntop", "comntop.col"),
);
await Deno.writeFile(
  join(comntopOutDir, "comntop.col"),
  concat([vanillaCol, buildAntennaColEntry("com_antenna", 2072)]),
);

const vanillaIde = await Deno.readTextFile(
  join(gta3Dir, "data", "maps", "comntop", "comntop.ide"),
);
const ideInsertion =
  "2072, com_antenna, com_antenna, 1, 299, 4\r\n" +
  "2073, LOD_antenna, com_antenna, 1, 1500, 4\r\n";
const patchedIde = vanillaIde.replace(
  /^objs\r?\n([\s\S]*?)\r?\nend\r?\n/m,
  (m, body) => {
    if (body.includes("com_antenna")) return m;
    return `objs\r\n${body}\r\n${ideInsertion}end\r\n`;
  },
);
if (patchedIde === vanillaIde) throw new Error("Failed to patch comntop.ide");
await Deno.writeTextFile(join(comntopOutDir, "comntop.ide"), patchedIde);

const vanillaIpl = await Deno.readTextFile(
  join(gta3Dir, "data", "maps", "comntop", "comNtop.ipl"),
);
const iplInsertion =
  "2072, com_antenna, 179.6, -300, 55.45, 1, 1, 1, 0, 0, 0, 1\r\n" +
  "2073, LOD_antenna, 179.6, -300, 55.45, 1, 1, 1, 0, 0, 0, 1\r\n";
const patchedIpl = vanillaIpl.replace(
  /^inst\r?\n([\s\S]*?)\r?\nend\r?\n/m,
  (m, body) => {
    if (body.includes("com_antenna")) return m;
    return `inst\r\n${body}\r\n${iplInsertion}end\r\n`;
  },
);
if (patchedIpl === vanillaIpl) throw new Error("Failed to patch comNtop.ipl");
await Deno.writeTextFile(join(comntopOutDir, "comNtop.ipl"), patchedIpl);

console.log("Generated com_antenna.dff, LOD_antenna.dff, com_antenna.txd, comntop.col, comntop.ide, comNtop.ipl");
