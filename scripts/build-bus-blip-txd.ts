/**
 * build-bus-blip-txd.ts
 *
 * Generates a crisp 64x64 GTA-III-style blue 'B' bus stop radar icon PNG
 * and packs it into a RenderWare 3.3 (0x0C02FFFF) .txd (eventide_busblip.txd).
 */

import { buildSingleTextureTxd, encodePngRgba } from "./build-menu-txd.ts";

// 9x13 bitmap glyph for bold 'B' (drawn scaled into the 64x64 badge)
const GLYPH_B = [
  "111111100",
  "111111110",
  "110000111",
  "110000011",
  "110000111",
  "111111110",
  "111111100",
  "111111110",
  "110000011",
  "110000011",
  "110000011",
  "111111110",
  "111111100",
];

function setPixel(
  buf: Uint8Array,
  size: number,
  x: number,
  y: number,
  r: number,
  g: number,
  b: number,
  a: number,
) {
  if (x < 0 || y < 0 || x >= size || y >= size) return;
  const idx = (y * size + x) * 4;
  const srcA = a / 255;
  const dstA = buf[idx + 3] / 255;
  const outA = srcA + dstA * (1 - srcA);
  if (outA <= 0) return;
  buf[idx] = Math.round((r * srcA + buf[idx] * dstA * (1 - srcA)) / outA);
  buf[idx + 1] = Math.round((g * srcA + buf[idx + 1] * dstA * (1 - srcA)) / outA);
  buf[idx + 2] = Math.round((b * srcA + buf[idx + 2] * dstA * (1 - srcA)) / outA);
  buf[idx + 3] = Math.round(outA * 255);
}

function roundedSquareDist(
  px: number,
  py: number,
  cx: number,
  cy: number,
  half: number,
  radius: number,
): number {
  const dx = Math.max(Math.abs(px - cx) - (half - radius), 0);
  const dy = Math.max(Math.abs(py - cy) - (half - radius), 0);
  return Math.hypot(dx, dy) - radius;
}

export async function buildBusBlipTxd(pngOut: string, txdOut: string) {
  const size = 64;
  const rgba = new Uint8Array(size * size * 4);
  const cx = 31.5;
  const cy = 31.5;

  // Rounded blue badge with dark outline and subtle top highlight
  for (let y = 0; y < size; y++) {
    for (let x = 0; x < size; x++) {
      const dOuter = roundedSquareDist(x, y, cx, cy, 26.0, 6.0);
      if (dOuter > 1.2) continue;
      const alpha = Math.max(0, Math.min(1, 1.0 - Math.max(0, dOuter)));
      const t = y / (size - 1);

      let r = Math.round(36 * (1 - t) + 14 * t);
      let g = Math.round(132 * (1 - t) + 74 * t);
      let b = Math.round(245 * (1 - t) + 188 * t);

      if (dOuter > -4.0) {
        // Crisp dark navy border
        r = 10;
        g = 30;
        b = 78;
      } else if (dOuter > -6.0) {
        // Subtle bright inner rim
        r = 108;
        g = 186;
        b = 255;
      }
      setPixel(rgba, size, x, y, r, g, b, Math.round(alpha * 255));
    }
  }

  // Crisp centered bold white 'B' with dark drop shadow
  const scale = 3;
  const gw = GLYPH_B[0].length * scale; // 27
  const gh = GLYPH_B.length * scale; // 39
  const gx0 = Math.round((size - gw) / 2);
  const gy0 = Math.round((size - gh) / 2);

  for (const pass of ["shadow", "fill"] as const) {
    const ox = pass === "shadow" ? 2 : 0;
    const oy = pass === "shadow" ? 2 : 0;
    for (let gy = 0; gy < GLYPH_B.length; gy++) {
      for (let gx = 0; gx < GLYPH_B[gy].length; gx++) {
        if (GLYPH_B[gy][gx] !== "1") continue;
        for (let sy = 0; sy < scale; sy++) {
          for (let sx = 0; sx < scale; sx++) {
            const px = gx0 + gx * scale + sx + ox;
            const py = gy0 + gy * scale + sy + oy;
            if (pass === "shadow") {
              setPixel(rgba, size, px, py, 8, 22, 58, 220);
            } else {
              setPixel(rgba, size, px, py, 255, 255, 255, 255);
            }
          }
        }
      }
    }
  }

  const pngBytes = await encodePngRgba(size, size, rgba);
  await Deno.writeFile(pngOut, pngBytes);
  await buildSingleTextureTxd("busblip", pngOut, txdOut);
}

if (import.meta.main) {
  await buildBusBlipTxd(
    "plugins/ShortRangeBlips/busblip64.png",
    "plugins/ShortRangeBlips/eventide_busblip.txd",
  );
  console.log("Generated busblip64.png and eventide_busblip.txd");
}
