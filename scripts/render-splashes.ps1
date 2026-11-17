param(
  [string]$BaseDir = (Join-Path $PSScriptRoot ".."),
  [string]$FontPath = (Join-Path $PSScriptRoot "..\plugins\MenuLogo\Caveat-Bold.ttf")
)

Add-Type -AssemblyName System.Drawing

$pfc = New-Object System.Drawing.Text.PrivateFontCollection
$pfc.AddFontFile((Resolve-Path $FontPath).Path)
$fam = $pfc.Families[0]
$style = [System.Drawing.FontStyle]::Regular
if (-not $fam.IsStyleAvailable($style)) {
  $style = [System.Drawing.FontStyle]::Bold
}

$sf = [System.Drawing.StringFormat]::GenericTypographic

function Get-LogoPath([float]$scale) {
  $em = 290.0 * $scale
  function Make-Path([string]$text, [float]$size) {
    $p = New-Object System.Drawing.Drawing2D.GraphicsPath
    $p.FillMode = [System.Drawing.Drawing2D.FillMode]::Winding
    $p.AddString($text, $fam, [int]$style, $size, [System.Drawing.PointF]::new(0, 0), $sf)
    return $p
  }

  $p1 = Make-Path "Journey" $em
  $p2 = Make-Path "Through" $em
  $p3 = Make-Path "Comatose" ($em * 1.04)

  $S = 1024.0 * $scale
  $padX = 48.0 * $scale
  $x1 = $padX + 16.0 * $scale
  $x2 = ($S - $padX) - $p2.GetBounds().Width - 8.0 * $scale
  $x3 = ($S - $p3.GetBounds().Width) * 0.5

  $y1 = 145.0 * $scale
  $y2 = 386.0 * $scale
  $y3 = 626.0 * $scale

  function Move-Path([System.Drawing.Drawing2D.GraphicsPath]$p, [float]$tx, [float]$ty) {
    $b = $p.GetBounds()
    $m = New-Object System.Drawing.Drawing2D.Matrix
    $m.Translate($tx - $b.X, $ty - $b.Y)
    $p.Transform($m)
    $m.Dispose()
  }

  Move-Path $p1 $x1 $y1
  Move-Path $p2 $x2 $y2
  Move-Path $p3 $x3 $y3

  $combined = New-Object System.Drawing.Drawing2D.GraphicsPath
  $combined.FillMode = [System.Drawing.Drawing2D.FillMode]::Winding
  $combined.AddPath($p1, $false)
  $combined.AddPath($p2, $false)
  $combined.AddPath($p3, $false)

  $p1.Dispose(); $p2.Dispose(); $p3.Dispose()
  return $combined
}

function Add-RoundedRect([System.Drawing.Drawing2D.GraphicsPath]$p, [float]$x, [float]$y, [float]$w, [float]$h, [float]$r) {
  $p.AddArc($x, $y, $r * 2, $r * 2, 180, 90)
  $p.AddArc($x + $w - $r * 2, $y, $r * 2, $r * 2, 270, 90)
  $p.AddArc($x + $w - $r * 2, $y + $h - $r * 2, $r * 2, $r * 2, 0, 90)
  $p.AddArc($x, $y + $h - $r * 2, $r * 2, $r * 2, 90, 90)
  $p.CloseFigure()
}

$scratchDir = "C:\Users\josie\.gemini\antigravity-acp\brain\bf4382a0-e467-4c7f-9688-0e3df4b52153\scratch"
$outDir = Join-Path $BaseDir "plugins\MenuLogo"

# --- 1. Build mainsc1.png (1024x1024) from orig_mainsc2.png ---
$mainscBmp = [System.Drawing.Bitmap]::FromFile("$scratchDir\orig_mainsc2.png")
$newMainsc = New-Object System.Drawing.Bitmap($mainscBmp.Width, $mainscBmp.Height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$gMain = [System.Drawing.Graphics]::FromImage($newMainsc)
$gMain.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$gMain.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$gMain.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
$gMain.DrawImage($mainscBmp, 0, 0, $mainscBmp.Width, $mainscBmp.Height)

# Mask out old small badge
$maskPath = New-Object System.Drawing.Drawing2D.GraphicsPath
Add-RoundedRect $maskPath 372.0 644.0 268.0 118.0 20.0
$darkBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 12, 14, 18))
$gMain.FillPath($darkBrush, $maskPath)
$darkBrush.Dispose()
$maskPath.Dispose()

# Draw Journey Through Comatose title logo
$logoPathMain = Get-LogoPath 0.37
$logoBoundsMain = $logoPathMain.GetBounds()
$targetCx = 505.0
$targetCy = 696.0
$transM = New-Object System.Drawing.Drawing2D.Matrix
$transM.Translate($targetCx - ($logoBoundsMain.X + $logoBoundsMain.Width * 0.5), $targetCy - ($logoBoundsMain.Y + $logoBoundsMain.Height * 0.5))
$logoPathMain.Transform($transM)
$transM.Dispose()

$penMain = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(255, 8, 10, 14), 18.0)
$penMain.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Round
$penMain.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
$penMain.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
$gMain.DrawPath($penMain, $logoPathMain)
$gMain.FillPath([System.Drawing.Brushes]::White, $logoPathMain)
$penMain.Dispose()
$logoPathMain.Dispose()

$mainscOut = Join-Path $outDir "mainsc1.png"
$newMainsc.Save($mainscOut, [System.Drawing.Imaging.ImageFormat]::Png)
$gMain.Dispose()
$newMainsc.Dispose()
$mainscBmp.Dispose()
Write-Output "Saved $mainscOut"

# --- 2. Build splash1.png (512x256) from orig_splash1.png ---
$splashBmp = [System.Drawing.Bitmap]::FromFile("$scratchDir\orig_splash1.png")
$newSplash = New-Object System.Drawing.Bitmap($splashBmp.Width, $splashBmp.Height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$gSplash = [System.Drawing.Graphics]::FromImage($newSplash)
$gSplash.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$gSplash.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$gSplash.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
$gSplash.DrawImage($splashBmp, 0, 0, $splashBmp.Width, $splashBmp.Height)

# Draw Caveat title logo on upper-left of splash1: x ~ [26..210], y ~ [14..125] (scale ~0.22)
$logoPathSplash = Get-LogoPath 0.22
$logoBoundsSplash = $logoPathSplash.GetBounds()
$targetSplashX = 26.0
$targetSplashY = 14.0
$transM2 = New-Object System.Drawing.Drawing2D.Matrix
$transM2.Translate($targetSplashX - $logoBoundsSplash.X, $targetSplashY - $logoBoundsSplash.Y)
$logoPathSplash.Transform($transM2)
$transM2.Dispose()

$penSplash = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(255, 6, 8, 12), 7.5)
$penSplash.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Round
$penSplash.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
$penSplash.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
$gSplash.DrawPath($penSplash, $logoPathSplash)
$gSplash.FillPath([System.Drawing.Brushes]::White, $logoPathSplash)
$penSplash.Dispose()
$logoPathSplash.Dispose()

$splashOut = Join-Path $outDir "splash1.png"
$newSplash.Save($splashOut, [System.Drawing.Imaging.ImageFormat]::Png)
$gSplash.Dispose()
$newSplash.Dispose()
$splashBmp.Dispose()
Write-Output "Saved $splashOut"

$pfc.Dispose()
