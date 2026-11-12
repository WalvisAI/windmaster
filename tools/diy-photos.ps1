# Prepares the 360 VIEW DIY photos in assets/img/diy/:
#  - shooter-view.jpg: pixelates the monitor in the top-left corner (readable chat text)
#  - every .jpg: downscaled to at most 1400 px on the long side, re-saved at quality 82
# Run from the repo root:  powershell -ExecutionPolicy Bypass -File tools/diy-photos.ps1
Add-Type -AssemblyName System.Drawing
$dir = Join-Path $PSScriptRoot "..\assets\img\diy"
$codec = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object { $_.MimeType -eq "image/jpeg" }
$params = New-Object System.Drawing.Imaging.EncoderParameters 1
$params.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter ([System.Drawing.Imaging.Encoder]::Quality, [long]82)

# Region to hide, as fractions of width/height (x, y, w, h).
$hide = @{ "shooter-view.jpg" = @(0.0, 0.0, 0.43, 0.40) }

Get-ChildItem $dir -Filter *.jpg | ForEach-Object {
  $src = [System.Drawing.Image]::FromFile($_.FullName)
  $scale = [Math]::Min(1.0, 1400.0 / [Math]::Max($src.Width, $src.Height))
  $w = [int]($src.Width * $scale); $h = [int]($src.Height * $scale)
  $out = New-Object System.Drawing.Bitmap $w, $h
  $g = [System.Drawing.Graphics]::FromImage($out)
  $g.InterpolationMode = "HighQualityBicubic"
  $g.DrawImage($src, 0, 0, $w, $h)
  $src.Dispose()

  if ($hide.ContainsKey($_.Name)) {
    $r = $hide[$_.Name]
    $rx = [int]($r[0] * $w); $ry = [int]($r[1] * $h); $rw = [int]($r[2] * $w); $rh = [int]($r[3] * $h)
    $rect = New-Object System.Drawing.Rectangle $rx, $ry, $rw, $rh
    $crop = $out.Clone($rect, $out.PixelFormat)
    # Pixelate: shrink to ~1/40 then blow back up with nearest neighbour, then soften.
    $tiny = New-Object System.Drawing.Bitmap ([Math]::Max(1, [int]($rw / 40))), ([Math]::Max(1, [int]($rh / 40)))
    $tg = [System.Drawing.Graphics]::FromImage($tiny); $tg.InterpolationMode = "HighQualityBilinear"
    $tg.DrawImage($crop, 0, 0, $tiny.Width, $tiny.Height); $tg.Dispose()
    $g.InterpolationMode = "HighQualityBilinear"
    $g.DrawImage($tiny, $rect)
    $tiny.Dispose(); $crop.Dispose()
  }
  $g.Dispose()
  $tmp = $_.FullName + ".tmp"
  $out.Save($tmp, $codec, $params); $out.Dispose()
  Move-Item -Force $tmp $_.FullName
  "{0}: {1}x{2}, {3:N0} KB" -f $_.Name, $w, $h, ((Get-Item $_.FullName).Length / 1KB)
}
