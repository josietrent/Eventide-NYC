import { basename, extname, join } from "@std/path";
import { ensureDir } from "@std/fs/ensure-dir";
import {
  buildRw33Txd,
  decodePngRgba,
  type TxdTextureInput,
} from "./build-menu-txd.ts";

/**
 * Packs a directory of PNG textures (`data/models/src/<model>/*.png`) into a
 * RenderWare 3.3 opaque 32-bit `.txd` (`data/models/gta3.img/<model>.txd`).
 * Each PNG filename stem becomes the RenderWare texture name.
 */
export async function buildTxdFromPngDir(
  pngDir: string,
  txdPath: string,
  opaque = true,
): Promise<string[]> {
  const pngFiles: string[] = [];
  for await (const entry of Deno.readDir(pngDir)) {
    if (entry.isFile && extname(entry.name).toLowerCase() === ".png") {
      pngFiles.push(entry.name);
    }
  }
  pngFiles.sort((a, b) => a.localeCompare(b));

  const textures: TxdTextureInput[] = [];
  for (const fileName of pngFiles) {
    const texName = basename(fileName, extname(fileName));
    const pngBytes = await Deno.readFile(join(pngDir, fileName));
    const decoded = await decodePngRgba(pngBytes);
    textures.push({
      name: texName,
      width: decoded.width,
      height: decoded.height,
      rgba: decoded.rgba,
      opaque,
    });
  }

  const txdBytes = buildRw33Txd(textures);
  await ensureDir(join(txdPath, ".."));
  const tmpPath = `${txdPath}.${Date.now()}.tmp`;
  await Deno.writeFile(tmpPath, txdBytes);
  try {
    await Deno.remove(txdPath);
  } catch {
    // Ignore if target does not exist
  }
  await Deno.rename(tmpPath, txdPath);
  return textures.map((t) => t.name);
}
