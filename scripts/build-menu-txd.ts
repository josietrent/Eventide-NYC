/**
 * build-menu-txd.ts
 *
 * Generates a default Eventide rounded-square badge PNG if missing,
 * and packs plugins/MenuLogo/eventidelogo128.png (and optional gtalogo128.png)
 * into a RenderWare 3.3 (0x0C02FFFF) 32-bit BGRA .txd (eventide_menu.txd) for GTA III.
 */

const RW_VER_GTA3 = 0x0c02ffff;

function crc32(bytes: Uint8Array): number {
  let c = 0xffffffff;
  for (let i = 0; i < bytes.length; i++) {
    c ^= bytes[i];
    for (let k = 0; k < 8; k++) {
      c = (c >>> 1) ^ (c & 1 ? 0xedb88320 : 0);
    }
  }
  return (c ^ 0xffffffff) >>> 0;
}

async function zlibCompress(raw: Uint8Array): Promise<Uint8Array> {
  const cs = new CompressionStream("deflate");
  const outPromise = new Response(cs.readable).arrayBuffer();
  const writer = cs.writable.getWriter();
  writer.write(new Uint8Array(raw));
  writer.close();
  return new Uint8Array(await outPromise);
}

async function zlibDecompress(compressed: Uint8Array): Promise<Uint8Array> {
  const ds = new DecompressionStream("deflate");
  const outPromise = new Response(ds.readable).arrayBuffer();
  const writer = ds.writable.getWriter();
  writer.write(new Uint8Array(compressed));
  writer.close();
  return new Uint8Array(await outPromise);
}

function pngChunk(type: string, data: Uint8Array): Uint8Array {
  const out = new Uint8Array(12 + data.length);
  const dv = new DataView(out.buffer);
  dv.setUint32(0, data.length, false);
  for (let i = 0; i < 4; i++) out[4 + i] = type.charCodeAt(i);
  out.set(data, 8);
  dv.setUint32(8 + data.length, crc32(out.subarray(4, 8 + data.length)), false);
  return out;
}

export async function encodePngRgba(
  width: number,
  height: number,
  rgba: Uint8Array,
): Promise<Uint8Array> {
  const ihdr = new Uint8Array(13);
  const dv = new DataView(ihdr.buffer);
  dv.setUint32(0, width, false);
  dv.setUint32(4, height, false);
  ihdr[8] = 8; // bit depth
  ihdr[9] = 6; // RGBA
  ihdr[10] = 0;
  ihdr[11] = 0;
  ihdr[12] = 0;

  const stride = width * 4;
  const raw = new Uint8Array(height * (1 + stride));
  for (let y = 0; y < height; y++) {
    raw[y * (1 + stride)] = 0; // filter None
    raw.set(rgba.subarray(y * stride, (y + 1) * stride), y * (1 + stride) + 1);
  }
  const idat = await zlibCompress(raw);

  const sig = new Uint8Array([137, 80, 78, 71, 13, 10, 26, 10]);
  const c1 = pngChunk("IHDR", ihdr);
  const c2 = pngChunk("IDAT", idat);
  const c3 = pngChunk("IEND", new Uint8Array(0));
  const out = new Uint8Array(sig.length + c1.length + c2.length + c3.length);
  let off = 0;
  for (const part of [sig, c1, c2, c3]) {
    out.set(part, off);
    off += part.length;
  }
  return out;
}

function paethPredictor(a: number, b: number, c: number): number {
  const p = a + b - c;
  const pa = Math.abs(p - a);
  const pb = Math.abs(p - b);
  const pc = Math.abs(p - c);
  if (pa <= pb && pa <= pc) return a;
  if (pb <= pc) return b;
  return c;
}

export async function decodePngRgba(bytes: Uint8Array): Promise<{
  width: number;
  height: number;
  rgba: Uint8Array;
}> {
  const sig = [137, 80, 78, 71, 13, 10, 26, 10];
  for (let i = 0; i < 8; i++) {
    if (bytes[i] !== sig[i]) {
      throw new Error("Invalid PNG signature");
    }
  }
  const dv = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  let pos = 8;
  let width = 0;
  let height = 0;
  let bitDepth = 0;
  let colorType = 0;
  const idatParts: Uint8Array[] = [];

  while (pos + 12 <= bytes.length) {
    const len = dv.getUint32(pos, false);
    const type = String.fromCharCode(
      bytes[pos + 4],
      bytes[pos + 5],
      bytes[pos + 6],
      bytes[pos + 7],
    );
    const data = bytes.subarray(pos + 8, pos + 8 + len);
    if (type === "IHDR") {
      const idv = new DataView(data.buffer, data.byteOffset, data.byteLength);
      width = idv.getUint32(0, false);
      height = idv.getUint32(4, false);
      bitDepth = data[8];
      colorType = data[9];
      if (bitDepth !== 8 || (colorType !== 6 && colorType !== 2)) {
        throw new Error(
          `Unsupported PNG format (bitDepth=${bitDepth}, colorType=${colorType}); expected 8-bit RGB or RGBA`,
        );
      }
      if (data[12] !== 0) {
        throw new Error("Interlaced PNG is not supported");
      }
    } else if (type === "IDAT") {
      idatParts.push(data);
    } else if (type === "IEND") {
      break;
    }
    pos += 12 + len;
  }

  const totalIdat = idatParts.reduce((s, p) => s + p.length, 0);
  const compressed = new Uint8Array(totalIdat);
  let coff = 0;
  for (const p of idatParts) {
    compressed.set(p, coff);
    coff += p.length;
  }
  const raw = await zlibDecompress(compressed);

  const bpp = colorType === 6 ? 4 : 3;
  const stride = width * bpp;
  const recon = new Uint8Array(height * stride);
  for (let y = 0; y < height; y++) {
    const filter = raw[y * (1 + stride)];
    const rowIn = y * (1 + stride) + 1;
    const rowOut = y * stride;
    const prevOut = (y - 1) * stride;
    for (let x = 0; x < stride; x++) {
      const val = raw[rowIn + x];
      const a = x >= bpp ? recon[rowOut + x - bpp] : 0;
      const b = y > 0 ? recon[prevOut + x] : 0;
      const c = y > 0 && x >= bpp ? recon[prevOut + x - bpp] : 0;
      let outByte = val;
      if (filter === 1) outByte = (val + a) & 0xff;
      else if (filter === 2) outByte = (val + b) & 0xff;
      else if (filter === 3) outByte = (val + ((a + b) >> 1)) & 0xff;
      else if (filter === 4) outByte = (val + paethPredictor(a, b, c)) & 0xff;
      recon[rowOut + x] = outByte;
    }
  }

  if (colorType === 6) {
    return { width, height, rgba: recon };
  }
  const rgba = new Uint8Array(width * height * 4);
  for (let i = 0, j = 0; i < width * height; i++, j += 3) {
    rgba[i * 4 + 0] = recon[j + 0];
    rgba[i * 4 + 1] = recon[j + 1];
    rgba[i * 4 + 2] = recon[j + 2];
    rgba[i * 4 + 3] = 255;
  }
  return { width, height, rgba };
}

function clamp01(x: number): number {
  return x < 0 ? 0 : x > 1 ? 1 : x;
}

function smoothstep(edge0: number, edge1: number, x: number): number {
  const t = clamp01((x - edge0) / (edge1 - edge0));
  return t * t * (3 - 2 * t);
}

function sdRoundedBox(
  px: number,
  py: number,
  cx: number,
  cy: number,
  hx: number,
  hy: number,
  r: number,
): number {
  const qx = Math.abs(px - cx) - hx + r;
  const qy = Math.abs(py - cy) - hy + r;
  const ox = Math.max(qx, 0);
  const oy = Math.max(qy, 0);
  return Math.hypot(ox, oy) + Math.min(Math.max(qx, qy), 0) - r;
}

function sdStar(
  px: number,
  py: number,
  cx: number,
  cy: number,
  outerR: number,
  innerR: number,
  points: number,
): number {
  const dx = px - cx;
  const dy = py - cy;
  const ang = Math.atan2(dy, dx) + Math.PI / 2;
  const sector = (2 * Math.PI) / points;
  const a = (((ang % sector) + sector) % sector) - sector * 0.5;
  const r = Math.hypot(dx, dy);
  const t = Math.abs(a) / (sector * 0.5);
  const edgeR = outerR * (1 - t) + innerR * t;
  return r - edgeR;
}

/**
 * Generates a crisp 128x128 Eventide rounded-square badge matching the style
 * and proportions of the bottom-corner Rockstar badges on GTA III's menu.
 */
function generateDefaultEventideLogo(): Uint8Array {
  const W = 128;
  const H = 128;
  const rgba = new Uint8Array(W * H * 4);

  const samples = [0.25, 0.75];
  for (let y = 0; y < H; y++) {
    for (let x = 0; x < W; x++) {
      let accR = 0;
      let accG = 0;
      let accB = 0;
      let accA = 0;

      for (const sy of samples) {
        for (const sx of samples) {
          const px = x + sx;
          const py = y + sy;

          // Outer rounded square filling 124x124 of the 128x128 canvas
          const boxDist = sdRoundedBox(px, py, 64, 64, 62, 62, 17);
          const alpha = smoothstep(0.9, -0.9, boxDist);

          // Base twilight indigo-teal gradient
          const gradT = clamp01((py - 4) / 120);
          let r = 34 * (1 - gradT) + 18 * gradT;
          let g = 68 * (1 - gradT) + 38 * gradT;
          let b = 104 * (1 - gradT) + 74 * gradT;

          // Dark outer frame
          if (boxDist > -6.0) {
            const borderT = smoothstep(-6.0, -4.5, boxDist);
            r = r * (1 - borderT) + 14 * borderT;
            g = g * (1 - borderT) + 20 * borderT;
            b = b * (1 - borderT) + 32 * borderT;
          }

          // Crescent moon + Eventide III three-star triad
          const moonOuter = Math.hypot(px - 56, py - 62) - 33;
          const moonCut = Math.hypot(px - 71, py - 56) - 28;
          const crescent = Math.max(moonOuter, -moonCut);

          const s1 = sdStar(px, py, 87, 43, 7.6, 3.2, 5);
          const s2 = sdStar(px, py, 96, 62, 6.2, 2.6, 5);
          const s3 = sdStar(px, py, 84, 80, 6.8, 2.9, 5);

          const emblemDist = Math.min(crescent, s1, s2, s3);

          const shadowDist = Math.min(
            Math.max(
              Math.hypot(px - 57.5, py - 64.5) - 33,
              -(Math.hypot(px - 72.5, py - 58.5) - 28),
            ),
            sdStar(px, py, 88.5, 45.5, 7.6, 3.2, 5),
            sdStar(px, py, 97.5, 64.5, 6.2, 2.6, 5),
            sdStar(px, py, 85.5, 82.5, 6.8, 2.9, 5),
          );
          const shadowCov = smoothstep(1.2, -1.2, shadowDist);
          r = r * (1 - shadowCov * 0.55) + 8 * (shadowCov * 0.55);
          g = g * (1 - shadowCov * 0.55) + 10 * (shadowCov * 0.55);
          b = b * (1 - shadowCov * 0.55) + 16 * (shadowCov * 0.55);

          const emblemCov = smoothstep(0.9, -0.9, emblemDist);
          const isStar = Math.min(s1, s2, s3) < crescent;
          const er = isStar ? 245 : 238;
          const eg = isStar ? 196 : 242;
          const eb = isStar ? 78 : 250;

          r = r * (1 - emblemCov) + er * emblemCov;
          g = g * (1 - emblemCov) + eg * emblemCov;
          b = b * (1 - emblemCov) + eb * emblemCov;

          accR += r * alpha;
          accG += g * alpha;
          accB += b * alpha;
          accA += alpha;
        }
      }

      const idx = (y * W + x) * 4;
      if (accA > 0) {
        rgba[idx + 0] = Math.round(accR / accA);
        rgba[idx + 1] = Math.round(accG / accA);
        rgba[idx + 2] = Math.round(accB / accA);
        rgba[idx + 3] = Math.round((accA / 4) * 255);
      } else {
        rgba[idx + 0] = 0;
        rgba[idx + 1] = 0;
        rgba[idx + 2] = 0;
        rgba[idx + 3] = 0;
      }
    }
  }
  return rgba;
}

function rwChunk(type: number, payload: Uint8Array): Uint8Array {
  const out = new Uint8Array(12 + payload.length);
  const dv = new DataView(out.buffer);
  dv.setUint32(0, type, true);
  dv.setUint32(4, payload.length, true);
  dv.setUint32(8, RW_VER_GTA3, true);
  out.set(payload, 12);
  return out;
}

function writeFixedCString(buf: Uint8Array, offset: number, str: string) {
  const enc = new TextEncoder().encode(str);
  buf.set(enc.subarray(0, 31), offset);
}

export interface TxdTextureInput {
  name: string;
  width: number;
  height: number;
  rgba: Uint8Array;
  opaque?: boolean;
}

function buildRwTextureNative(tex: TxdTextureInput): Uint8Array {
  const pixelBytes = tex.width * tex.height * 4;
  const structPayload = new Uint8Array(88 + 4 + pixelBytes);
  const sdv = new DataView(structPayload.buffer);

  // RwTextureNative D3D8 raster struct (matches GTA III PC menu.txd / splash.txd)
  sdv.setUint32(0, 8, true); // platformId = D3D8
  sdv.setUint32(4, 0x1102, true); // filter=LINEAR, uAddressing=WRAP, vAddressing=WRAP
  writeFixedCString(structPayload, 8, tex.name);
  writeFixedCString(structPayload, 40, ""); // maskName empty -> uses RGBA alpha
  sdv.setUint32(72, 0x0500, true); // RASTER_8888
  sdv.setUint32(76, tex.opaque ? 0 : 1, true); // hasAlpha
  sdv.setUint16(80, tex.width, true);
  sdv.setUint16(82, tex.height, true);
  structPayload[84] = 32; // bpp
  structPayload[85] = 1; // mipmapCount
  structPayload[86] = 4; // rasterType = TEXTURE
  structPayload[87] = 0; // compression = none (D3DFMT_A8R8G8B8)
  sdv.setUint32(88, pixelBytes, true);

  // Convert RGBA -> BGRA
  for (let i = 0; i < tex.width * tex.height; i++) {
    const src = i * 4;
    const dst = 92 + src;
    structPayload[dst + 0] = tex.rgba[src + 2]; // B
    structPayload[dst + 1] = tex.rgba[src + 1]; // G
    structPayload[dst + 2] = tex.rgba[src + 0]; // R
    structPayload[dst + 3] = tex.opaque ? 255 : tex.rgba[src + 3]; // A
  }

  const texStructChunk = rwChunk(0x01, structPayload);
  const texExtChunk = rwChunk(0x03, new Uint8Array(0));
  const texNativePayload = new Uint8Array(
    texStructChunk.length + texExtChunk.length,
  );
  texNativePayload.set(texStructChunk, 0);
  texNativePayload.set(texExtChunk, texStructChunk.length);
  return rwChunk(0x15, texNativePayload);
}

export function buildRw33Txd(textures: TxdTextureInput[]): Uint8Array {
  const nativeChunks = textures.map(buildRwTextureNative);

  const dictHeader = new Uint8Array(4);
  new DataView(dictHeader.buffer).setUint16(0, textures.length, true);
  const dictStructChunk = rwChunk(0x01, dictHeader);
  const dictExtChunk = rwChunk(0x03, new Uint8Array(0));

  const totalSize =
    dictStructChunk.length +
    nativeChunks.reduce((sum, c) => sum + c.length, 0) +
    dictExtChunk.length;
  const txdPayload = new Uint8Array(totalSize);
  let off = 0;
  txdPayload.set(dictStructChunk, off);
  off += dictStructChunk.length;
  for (const chunk of nativeChunks) {
    txdPayload.set(chunk, off);
    off += chunk.length;
  }
  txdPayload.set(dictExtChunk, off);

  return rwChunk(0x16, txdPayload);
}

export async function buildSingleTextureTxd(
  texName: string,
  pngPath: string,
  txdPath: string,
) {
  const pngBytes = await Deno.readFile(pngPath);
  const { width, height, rgba } = await decodePngRgba(pngBytes);
  const txdBytes = buildRw33Txd([{ name: texName, width, height, rgba }]);
  const tmpPath = `${txdPath}.${Date.now()}.tmp`;
  await Deno.writeFile(tmpPath, txdBytes);
  try {
    await Deno.remove(txdPath);
  } catch {
    // Ignore if file does not exist
  }
  await Deno.rename(tmpPath, txdPath);
}

export async function buildMenuTxd(
  badgePngPath: string,
  txdPath: string,
  titlePngPath?: string,
) {
  let badgePngBytes: Uint8Array;
  try {
    badgePngBytes = await Deno.readFile(badgePngPath);
  } catch {
    const defaultRgba = generateDefaultEventideLogo();
    badgePngBytes = await encodePngRgba(128, 128, defaultRgba);
    await Deno.writeFile(badgePngPath, badgePngBytes);
  }

  const textures: TxdTextureInput[] = [];
  const badge = await decodePngRgba(badgePngBytes);
  textures.push({ name: "eventidelogo128", ...badge });

  if (titlePngPath) {
    try {
      const titlePngBytes = await Deno.readFile(titlePngPath);
      const title = await decodePngRgba(titlePngBytes);
      textures.push({ name: "gtaLogo128", ...title });
    } catch {
      // Optional title override PNG not present
    }
  }

  const txdBytes = buildRw33Txd(textures);
  await Deno.writeFile(txdPath, txdBytes);
}
