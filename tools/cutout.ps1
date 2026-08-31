<#
  cutout.ps1 — knock the flat studio background out of a product photo and write a
  tight, transparent PNG for assets/img/hardware/.

  The vendor shots (and the copies that ship as app drawables) sit on flat white or
  light grey. Dropped straight onto the site they would be bright rectangles in the
  dark theme, so a meter card gets a cut-out instead of the white plate .shop-plate
  gives a brand mark.

  How it works, and why not a simple colour key: the fill starts from the BORDER and
  walks inwards, so light areas enclosed by the product — the white between the
  WEATHERmeter's impeller blades — stay opaque. Boundary pixels get a feathered alpha
  and their colour un-mixed (observed = a*F + (1-a)*bg, solved for F), which is what
  stops a pale halo appearing once the shot lands on a dark surface.

    .\tools\cutout.ps1 -In ..\FieldTargetWind\composeApp\src\androidMain\res\drawable\calypso.png `
                       -Out .\assets\img\hardware\calypso.png

  Raise -T2 if background survives around the edges, lower it if the product is being
  eaten into. Windows only (System.Drawing); no ImageMagick needed.
#>
[CmdletBinding()]
param(
  [Parameter(Mandatory = $true)][string]$In,
  [Parameter(Mandatory = $true)][string]$Out,
  # Below T1 from the sampled background colour a pixel is fully transparent; between
  # T1 and T2 it fades in. Above T2 the fill stops, so T2 is the edge of the product.
  [int]$T1 = 22,
  [int]$T2 = 70,
  # Transparent margin left around the trimmed subject.
  [int]$Pad = 6
)

Add-Type -AssemblyName System.Drawing

$src = @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class Cutout {
  public static void Run(string inPath, string outPath, int t1, int t2, int pad) {
    Bitmap src = new Bitmap(inPath);
    int w = src.Width, h = src.Height;
    Bitmap bmp = new Bitmap(w, h, PixelFormat.Format32bppArgb);
    using (Graphics g = Graphics.FromImage(bmp)) { g.DrawImage(src, 0, 0, w, h); }
    src.Dispose();

    BitmapData bd = bmp.LockBits(new Rectangle(0, 0, w, h), ImageLockMode.ReadWrite, PixelFormat.Format32bppArgb);
    int stride = bd.Stride;
    byte[] px = new byte[stride * h];
    Marshal.Copy(bd.Scan0, px, 0, px.Length);

    // The top-left pixel is the background sample; these shots are always flat there.
    int bgB = px[0], bgG = px[1], bgR = px[2];

    bool[] outside = new bool[w * h];
    int[] queue = new int[w * h];
    int qh = 0, qt = 0;

    for (int x = 0; x < w; x++) { Seed(px, stride, outside, queue, ref qt, x, 0, w, bgR, bgG, bgB, t2);
                                  Seed(px, stride, outside, queue, ref qt, x, h - 1, w, bgR, bgG, bgB, t2); }
    for (int y = 0; y < h; y++) { Seed(px, stride, outside, queue, ref qt, 0, y, w, bgR, bgG, bgB, t2);
                                  Seed(px, stride, outside, queue, ref qt, w - 1, y, w, bgR, bgG, bgB, t2); }

    while (qh < qt) {
      int idx = queue[qh++];
      int x = idx % w, y = idx / w;
      if (x > 0)     Seed(px, stride, outside, queue, ref qt, x - 1, y, w, bgR, bgG, bgB, t2);
      if (x < w - 1) Seed(px, stride, outside, queue, ref qt, x + 1, y, w, bgR, bgG, bgB, t2);
      if (y > 0)     Seed(px, stride, outside, queue, ref qt, x, y - 1, w, bgR, bgG, bgB, t2);
      if (y < h - 1) Seed(px, stride, outside, queue, ref qt, x, y + 1, w, bgR, bgG, bgB, t2);
    }

    int minX = w, minY = h, maxX = -1, maxY = -1;
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        int o = y * stride + x * 4;
        int a = 255;
        if (outside[y * w + x]) {
          int d = Diff(px[o + 2], px[o + 1], px[o], bgR, bgG, bgB);
          if (d <= t1) a = 0;
          else a = (int)(255.0 * (d - t1) / (double)(t2 - t1));
          if (a > 255) a = 255;
          if (a > 0) {
            double af = a / 255.0;
            px[o + 2] = Unmix(px[o + 2], bgR, af);
            px[o + 1] = Unmix(px[o + 1], bgG, af);
            px[o]     = Unmix(px[o],     bgB, af);
          }
          px[o + 3] = (byte)a;
        } else {
          px[o + 3] = 255;
        }
        if (px[o + 3] > 8) {
          if (x < minX) minX = x; if (x > maxX) maxX = x;
          if (y < minY) minY = y; if (y > maxY) maxY = y;
        }
      }
    }

    Marshal.Copy(px, 0, bd.Scan0, px.Length);
    bmp.UnlockBits(bd);

    minX = Math.Max(0, minX - pad); minY = Math.Max(0, minY - pad);
    maxX = Math.Min(w - 1, maxX + pad); maxY = Math.Min(h - 1, maxY + pad);
    Rectangle crop = new Rectangle(minX, minY, maxX - minX + 1, maxY - minY + 1);

    Bitmap outBmp = bmp.Clone(crop, PixelFormat.Format32bppArgb);
    outBmp.Save(outPath, ImageFormat.Png);
    Console.WriteLine(inPath + " -> " + outPath + "  " + crop.Width + "x" + crop.Height +
                      "  bg=(" + bgR + "," + bgG + "," + bgB + ")");
    outBmp.Dispose();
    bmp.Dispose();
  }

  static byte Unmix(byte c, int bg, double a) {
    double f = (c - (1.0 - a) * bg) / a;
    if (f < 0) f = 0; if (f > 255) f = 255;
    return (byte)Math.Round(f);
  }

  static int Diff(int r, int g, int b, int br, int bg2, int bb) {
    int dr = Math.Abs(r - br), dg = Math.Abs(g - bg2), db = Math.Abs(b - bb);
    int m = dr > dg ? dr : dg;
    return m > db ? m : db;
  }

  static void Seed(byte[] px, int stride, bool[] outside, int[] queue, ref int qt,
                   int x, int y, int w, int br, int bg2, int bb, int t2) {
    int i = y * w + x;
    if (outside[i]) return;
    int o = y * stride + x * 4;
    if (Diff(px[o + 2], px[o + 1], px[o], br, bg2, bb) > t2) return;
    outside[i] = true;
    queue[qt++] = i;
  }
}
'@

Add-Type -TypeDefinition $src -ReferencedAssemblies System.Drawing

$inFull = (Resolve-Path $In).Path
$outDir = Split-Path -Parent $Out
if ($outDir -and -not (Test-Path $outDir)) { New-Item -ItemType Directory -Path $outDir | Out-Null }
$outFull = [System.IO.Path]::GetFullPath([System.IO.Path]::Combine((Get-Location).Path, $Out))

[Cutout]::Run($inFull, $outFull, $T1, $T2, $Pad)
