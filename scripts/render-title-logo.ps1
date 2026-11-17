param(
  [string]$FontPath = (Join-Path $PSScriptRoot "..\plugins\MenuLogo\Caveat-Bold.ttf"),
  [string]$OutPath = (Join-Path $PSScriptRoot "..\plugins\MenuLogo\gtalogo128.png")
)

Add-Type -AssemblyName System.Drawing

$pfc = New-Object System.Drawing.Text.PrivateFontCollection
$pfc.AddFontFile((Resolve-Path $FontPath).Path)
$fam = $pfc.Families[0]
$style = [System.Drawing.FontStyle]::Regular
if (-not $fam.IsStyleAvailable($style)) {
  $style = [System.Drawing.FontStyle]::Bold
}

$S = 1024
$bmp = New-Object System.Drawing.Bitmap($S, $S, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
$g.Clear([System.Drawing.Color]::Transparent)

$sf = [System.Drawing.StringFormat]::GenericTypographic

function Get-WordPath([string]$text, [float]$emSize) {
  $p = New-Object System.Drawing.Drawing2D.GraphicsPath
  $p.FillMode = [System.Drawing.Drawing2D.FillMode]::Winding
  $p.AddString($text, $fam, [int]$style, $emSize, [System.Drawing.PointF]::new(0, 0), $sf)
  return $p
}

$em = 290.0
$p1 = Get-WordPath "Journey" $em
$p2 = Get-WordPath "Through" $em
$p3 = Get-WordPath "Comatose" ($em * 1.04)

$b1 = $p1.GetBounds()
$b2 = $p2.GetBounds()
$b3 = $p3.GetBounds()

# Match GTA III menu logo safe padding (~14% top/bottom, ~5% left/right) and
# staggered 3-line lockup:
#   Line 1 (Journey):  upper-left
#   Line 2 (Through):  middle-right
#   Line 3 (Comatose): lower centered/full-width
$padX = 48.0
$x1 = $padX + 16.0
$x2 = ($S - $padX) - $b2.Width - 8.0
$x3 = ($S - $b3.Width) * 0.5

$y1 = 145.0
$y2 = 386.0
$y3 = 626.0

function Draw-Word([System.Drawing.Drawing2D.GraphicsPath]$srcPath, [float]$tx, [float]$ty) {
  $b = $srcPath.GetBounds()
  $m = New-Object System.Drawing.Drawing2D.Matrix
  $m.Translate($tx - $b.X, $ty - $b.Y)
  $p = $srcPath.Clone()
  $p.FillMode = [System.Drawing.Drawing2D.FillMode]::Winding
  $p.Transform($m)

  $pen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(255, 8, 10, 14), 30.0)
  $pen.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Round
  $pen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
  $pen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
  $g.DrawPath($pen, $p)
  $g.FillPath([System.Drawing.Brushes]::White, $p)

  $pen.Dispose()
  $p.Dispose()
  $m.Dispose()
}

Draw-Word $p1 $x1 $y1
Draw-Word $p2 $x2 $y2
Draw-Word $p3 $x3 $y3

$out = New-Object System.Drawing.Bitmap(512, 512, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g2 = [System.Drawing.Graphics]::FromImage($out)
$g2.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g2.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$g2.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
$g2.DrawImage($bmp, 0, 0, 512, 512)
$out.Save($OutPath, [System.Drawing.Imaging.ImageFormat]::Png)

$g2.Dispose()
$out.Dispose()
$g.Dispose()
$bmp.Dispose()
$pfc.Dispose()
