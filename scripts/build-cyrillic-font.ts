/**
 * build-cyrillic-font.ts
 *
 * Rasterizes Cyrillic (and related typography) glyphs from plugins/MenuLogo/Caveat-Bold.ttf
 * into a 1024x512 RenderWare 3.3 .txd sprite atlas (eventide_cyrillic.txd) and emits
 * glyph metrics in plugins/CyrillicText/cyrillic_atlas.h for CyrillicText.asi.
 */

import { dirname, join, resolve } from "@std/path";
import { ensureDir } from "@std/fs/ensure-dir";
import { buildRw33Txd } from "./build-menu-txd.ts";

interface GlyphMetric {
  codepoint: number;
  u0: number;
  v0: number;
  u1: number;
  v1: number;
  xOffUnits: number;
  yOffUnits: number;
  quadWUnits: number;
  quadHUnits: number;
  advanceUnits: number;
}

const CODEPOINTS: number[] = [
  // Russian uppercase & lowercase
  0x0401, // Ё
  ...Array.from({ length: 32 }, (_, i) => 0x0410 + i), // А-Я
  ...Array.from({ length: 32 }, (_, i) => 0x0430 + i), // а-я
  0x0451, // ё
  // Ukrainian / Belarusian extras
  0x0404,
  0x0454, // Є є
  0x0406,
  0x0456, // І і
  0x0407,
  0x0457, // Ї ї
  0x040e,
  0x045e, // Ў ў
  0x0490,
  0x0491, // Ґ ґ
  // Cyrillic punctuation / typography
  0x00ab,
  0x00bb, // « »
  0x2013,
  0x2014, // – —
  0x2116, // №
].sort((a, b) => a - b);

function resolvePowerShellExe(): string {
  let sysRoot = "C:\\Windows";
  try {
    sysRoot = Deno.env.get("SystemRoot") || Deno.env.get("WINDIR") || sysRoot;
  } catch {
    // --allow-env may not be granted when run standalone; fall back to C:\Windows
  }
  const candidates = [
    join(sysRoot, "System32", "WindowsPowerShell", "v1.0", "powershell.exe"),
    join(sysRoot, "Sysnative", "WindowsPowerShell", "v1.0", "powershell.exe"),
  ];
  for (const exe of candidates) {
    try {
      if (Deno.statSync(exe).isFile) return exe;
    } catch {
      // Try next candidate
    }
  }
  return "powershell.exe";
}
export async function buildCyrillicFontAtlas(
  ttfPath: string,
  txdPath: string,
  headerPath: string,
) {
  const absTtf = resolve(ttfPath);
  const absTmpDir = await Deno.makeTempDir({ prefix: "eventide_cyr_" });
  const rawAtlasPath = join(absTmpDir, "atlas.raw");
  const metricsJsonPath = join(absTmpDir, "metrics.json");
  const psScriptPath = join(absTmpDir, "render_atlas.ps1");

  const atlasW = 1024;
  const atlasH = 512;
  const cellW = 64;
  const cellH = 80;
  const cols = 16;
  const emSize = 40.0;
  const baselineY = 52.0;
  const padX = 10.0;

  // GTA III FONT_BANK uses ~20 * scaleY tall cells while scaleX is typically ~0.43..0.5x scaleY.
  // Scaling X at ~2.0x Y keeps Caveat proportions natural and aligns glyph widths with Latin (~21-26 units).
  const pxToHeightUnits = 23.8 / emSize;
  const pxToWidthUnits = 48.0 / emSize;

  const psScript = `
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName PresentationCore
Add-Type -AssemblyName WindowsBase

$ttfPath = '${absTtf.replace(/'/g, "''")}'
$rawOut  = '${rawAtlasPath.replace(/'/g, "''")}'
$jsonOut = '${metricsJsonPath.replace(/'/g, "''")}'

$gt = New-Object System.Windows.Media.GlyphTypeface([uri]$ttfPath)
$atlasW = ${atlasW}
$atlasH = ${atlasH}
$cellW = ${cellW}
$cellH = ${cellH}
$cols = ${cols}
$emSize = [double]${emSize}
$baselineY = [double]${baselineY}
$padX = [double]${padX}

$codepoints = @(${CODEPOINTS.join(",")})

$dv = New-Object System.Windows.Media.DrawingVisual
$dc = $dv.RenderOpen()

$metrics = @()
for ($idx = 0; $idx -lt $codepoints.Count; $idx++) {
  $cp = [int]$codepoints[$idx]
  $col = $idx % $cols
  $row = [math]::Floor($idx / $cols)
  $cx = $col * $cellW
  $cy = $row * $cellH

  if (-not $gt.CharacterToGlyphMap.ContainsKey($cp)) {
    throw ("Font does not contain glyph for U+{0:X4}" -f $cp)
  }

  $glyphIdx = $gt.CharacterToGlyphMap[$cp]
  $advPx = $gt.AdvanceWidths[$glyphIdx] * $emSize
  $lsbPx = $gt.LeftSideBearings[$glyphIdx] * $emSize
  $rsbPx = $gt.RightSideBearings[$glyphIdx] * $emSize
  $inkW = $advPx - $lsbPx - $rsbPx

  # Shift origin so negative left-side bearings stay inside the cell padding
  $drawBiasX = $padX
  if ($lsbPx -lt 0) {
    $drawBiasX = $padX - $lsbPx
  }
  if (($drawBiasX + $lsbPx + $inkW) -gt ($cellW - 2)) {
    throw ("Glyph U+{0:X4} overflows cellW ({1} + {2} + {3} > {4})" -f $cp, $drawBiasX, $lsbPx, $inkW, $cellW)
  }

  $origin = New-Object System.Windows.Point(($cx + $drawBiasX), ($cy + $baselineY))
  $glyphList = New-Object 'System.Collections.Generic.List[System.UInt16]'
  $glyphList.Add([uint16]$glyphIdx)
  $advList = New-Object 'System.Collections.Generic.List[System.Double]'
  $advList.Add($advPx)
  $offList = New-Object 'System.Collections.Generic.List[System.Windows.Point]'
  $offList.Add((New-Object System.Windows.Point(0, 0)))

  $glyphRun = New-Object System.Windows.Media.GlyphRun(
    $gt,
    0,
    $false,
    $emSize,
    $glyphList,
    $origin,
    $advList,
    $offList,
    $null,
    $null,
    $null,
    $null,
    $null
  )
  $dc.DrawGlyphRun([System.Windows.Media.Brushes]::White, $glyphRun)

  $metrics += [pscustomobject]@{
    codepoint = $cp
    cellX = $cx
    cellY = $cy
    drawBiasX = $drawBiasX
    baselineY = $baselineY
    advPx = $advPx
  }
}

$dc.Close()

$rtb = New-Object System.Windows.Media.Imaging.RenderTargetBitmap(
  $atlasW, $atlasH, 96, 96, [System.Windows.Media.PixelFormats]::Pbgra32
)
$rtb.Render($dv)

$stride = $atlasW * 4
$bgra = New-Object byte[] ($stride * $atlasH)
$rtb.CopyPixels($bgra, $stride, 0)

# Convert premultiplied BGRA -> straight white RGBA (255,255,255,A) for clean vertex tinting
$rgba = New-Object byte[] ($stride * $atlasH)
for ($i = 0; $i -lt $bgra.Length; $i += 4) {
  $a = $bgra[$i + 3]
  if ($a -gt 0) {
    $rgba[$i + 0] = 255
    $rgba[$i + 1] = 255
    $rgba[$i + 2] = 255
    $rgba[$i + 3] = $a
  }
}

[System.IO.File]::WriteAllBytes($rawOut, $rgba)
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($jsonOut, ($metrics | ConvertTo-Json -Compress), $utf8NoBom)
`;

  try {
    await Deno.writeTextFile(psScriptPath, psScript);
    const cmd = new Deno.Command(resolvePowerShellExe(), {
      args: [
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-STA",
        "-File",
        psScriptPath,
      ],
    });
    const out = await cmd.output();
    if (!out.success) {
      const err = new TextDecoder().decode(out.stderr);
      throw new Error(`PowerShell glyph rasterization failed:\n${err}`);
    }

    const rgba = await Deno.readFile(rawAtlasPath);
    const rawJson = (await Deno.readTextFile(metricsJsonPath)).replace(
      /^\uFEFF/,
      "",
    );
    const rawMetrics = JSON.parse(rawJson) as Array<{
      codepoint: number;
      cellX: number;
      cellY: number;
      drawBiasX: number;
      baselineY: number;
      advPx: number;
    }>;

    // Normalize GTA III metrics relative to the 20-unit FONT_BANK cell (baseline ~16.5 down)
    const gtaBaselineOffsetUnits = 16.5;
    const glyphMetrics: GlyphMetric[] = rawMetrics.map((m) => {
      // Half-texel inset keeps sampling crisp inside the cell
      const u0 = (m.cellX + 0.5) / atlasW;
      const v0 = (m.cellY + 0.5) / atlasH;
      const u1 = (m.cellX + cellW - 0.5) / atlasW;
      const v1 = (m.cellY + cellH - 0.5) / atlasH;

      return {
        codepoint: m.codepoint,
        u0,
        v0,
        u1,
        v1,
        xOffUnits: -m.drawBiasX * pxToWidthUnits,
        yOffUnits: gtaBaselineOffsetUnits - m.baselineY * pxToHeightUnits,
        quadWUnits: cellW * pxToWidthUnits,
        quadHUnits: cellH * pxToHeightUnits,
        advanceUnits: m.advPx * pxToWidthUnits,
      };
    });

    await ensureDir(dirname(txdPath));
    await ensureDir(dirname(headerPath));

    const txdBytes = buildRw33Txd([
      { name: "cyrillic", width: atlasW, height: atlasH, rgba },
    ]);
    const tmpTxd = `${txdPath}.${Date.now()}.tmp`;
    await Deno.writeFile(tmpTxd, txdBytes);
    try {
      await Deno.remove(txdPath);
    } catch {
      // Ignore if missing
    }
    await Deno.rename(tmpTxd, txdPath);

    const rows = glyphMetrics
      .map(
        (g) =>
          `  {0x${g.codepoint.toString(16).toUpperCase().padStart(4, "0")}, ` +
          `${g.u0.toFixed(6)}f, ${g.v0.toFixed(6)}f, ${g.u1.toFixed(6)}f, ${g.v1.toFixed(6)}f, ` +
          `${g.xOffUnits.toFixed(4)}f, ${g.yOffUnits.toFixed(4)}f, ` +
          `${g.quadWUnits.toFixed(4)}f, ${g.quadHUnits.toFixed(4)}f, ` +
          `${g.advanceUnits.toFixed(4)}f},`,
      )
      .join("\n");

    const header = `// Auto-generated by scripts/build-cyrillic-font.ts from Caveat-Bold.ttf
#pragma once
#include <stddef.h>
#include <stdint.h>

struct CyrillicGlyphMetric {
  uint16_t codepoint;
  float u0;
  float v0;
  float u1;
  float v1;
  float xOffUnits;
  float yOffUnits;
  float quadWUnits;
  float quadHUnits;
  float advanceUnits;
};

constexpr CyrillicGlyphMetric kCyrillicGlyphs[] = {
${rows}
};

constexpr size_t kCyrillicGlyphCount =
    sizeof(kCyrillicGlyphs) / sizeof(kCyrillicGlyphs[0]);
`;
    await Deno.writeTextFile(headerPath, header);
  } finally {
    await Deno.remove(absTmpDir, { recursive: true }).catch(() => {});
  }
}

if (import.meta.main) {
  const cwd = Deno.cwd();
  await buildCyrillicFontAtlas(
    join(cwd, "plugins", "MenuLogo", "Caveat-Bold.ttf"),
    join(cwd, "plugins", "CyrillicText", "eventide_cyrillic.txd"),
    join(cwd, "plugins", "CyrillicText", "cyrillic_atlas.h"),
  );
  console.log("Generated eventide_cyrillic.txd and cyrillic_atlas.h");
}
