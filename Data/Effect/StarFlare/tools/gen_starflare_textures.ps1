# Procedural textures for the StarFlare effect (engine cross flare).
# Colors are baked into RGB (white root -> cyan tip); the node color only scales brightness.
# Usage: powershell -NoProfile -ExecutionPolicy Bypass -File gen_starflare_textures.ps1 <outDir> [previewPath]
param([string]$OutDir, [string]$Preview = "")

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class StarFlareTex
{
    static float C01(float x) { return x < 0 ? 0 : (x > 1 ? 1 : x); }
    static float Lerp(float a, float b, float t) { return a + (b - a) * t; }

    // rgba: 4 floats per pixel, 0..1, straight (non-premultiplied) alpha
    static void Save(float[] rgba, int w, int h, string path)
    {
        var bmp = new Bitmap(w, h, PixelFormat.Format32bppArgb);
        var data = bmp.LockBits(new Rectangle(0, 0, w, h), ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
        byte[] buf = new byte[w * h * 4];
        for (int i = 0; i < w * h; i++)
        {
            buf[i * 4 + 0] = (byte)(C01(rgba[i * 4 + 2]) * 255); // B
            buf[i * 4 + 1] = (byte)(C01(rgba[i * 4 + 1]) * 255); // G
            buf[i * 4 + 2] = (byte)(C01(rgba[i * 4 + 0]) * 255); // R
            buf[i * 4 + 3] = (byte)(C01(rgba[i * 4 + 3]) * 255); // A
        }
        Marshal.Copy(buf, 0, data.Scan0, buf.Length);
        bmp.UnlockBits(data);
        bmp.Save(path, ImageFormat.Png);
        bmp.Dispose();
    }

    // Root color (near the core) and tip color (ray ends). Tip is the engine-emission cyan.
    static readonly float[] RootCol = { 0.82f, 0.93f, 1.00f };
    static readonly float[] TipCol  = { 0.12f, 0.62f, 1.00f };

    // Ray: thin spindle along X. Thick at the center (root), 0 width and 0 alpha at the tips.
    // The tips end at |x| = TipEnd so the quad edge is fully transparent.
    public static void Ray(string path, int w, int h)
    {
        const float TipEnd = 0.94f;
        const float RootHalfWidth = 0.40f; // fraction of the image half-height
        float[] p = new float[w * h * 4];
        for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
        {
            float x = (i + 0.5f) / w * 2 - 1;
            float y = (j + 0.5f) / h * 2 - 1;
            float a = Math.Abs(x) / TipEnd;            // 0 at root, 1 at tip
            float alpha = 0, t = 1;
            if (a < 1)
            {
                float halfW = RootHalfWidth * (float)Math.Pow(1 - a, 1.1) + 0.02f;
                float s = Math.Abs(y) / halfW;          // 0 on the center line, 1 at the line edge
                // Soft bell profile (no hard spindle outline, so a tilted plate does not read as a blade)
                float line = (float)Math.Exp(-s * s * 3.0);
                float halo = 0.04f * (float)Math.Pow(C01(1 - s / 2.2f), 2);
                float along = (float)Math.Pow(1 - a, 1.3);
                alpha = (line + halo) * along * C01((1 - Math.Abs(y)) * 8);
                t = C01((float)Math.Pow(a, 0.5) + 0.5f * C01(s) * C01(s));
            }
            int k = (j * w + i) * 4;
            p[k + 0] = Lerp(RootCol[0], TipCol[0], t);
            p[k + 1] = Lerp(RootCol[1], TipCol[1], t);
            p[k + 2] = Lerp(RootCol[2], TipCol[2], t);
            p[k + 3] = alpha;
        }
        Save(p, w, h, path);
    }

    // Vertical copy of a ray texture. Billboard sprites ignore node rotation, so the vertical ray needs its own image.
    public static void RotateToVertical(string srcPath, string dstPath)
    {
        using (var bmp = new Bitmap(srcPath))
        {
            bmp.RotateFlip(RotateFlipType.Rotate90FlipNone);
            bmp.Save(dstPath, ImageFormat.Png);
        }
    }

    // Core: smooth white peak with a thin pale-blue bloom. No disc edge, so a tilted plate does not read as a coin.
    public static void Core(string path, int n)
    {
        float[] p = new float[n * n * 4];
        for (int j = 0; j < n; j++)
        for (int i = 0; i < n; i++)
        {
            float x = (i + 0.5f) / n * 2 - 1, y = (j + 0.5f) / n * 2 - 1;
            float r = (float)Math.Sqrt(x * x + y * y);
            float disc = (float)Math.Exp(-(r / 0.28f) * (r / 0.28f));
            float bloom = 0.20f * (float)Math.Pow(C01(1 - r), 2.5);
            float t = C01((r - 0.15f) / 0.45f);
            int k = (j * n + i) * 4;
            p[k + 0] = Lerp(1.0f, 0.60f, t);
            p[k + 1] = Lerp(1.0f, 0.85f, t);
            p[k + 2] = 1.0f;
            p[k + 3] = C01(disc + bloom) * C01((1 - r) * 6);
        }
        Save(p, n, n, path);
    }

    // Glow: very soft pale-blue circle, 0 at the edge.
    public static void Glow(string path, int n)
    {
        float[] p = new float[n * n * 4];
        for (int j = 0; j < n; j++)
        for (int i = 0; i < n; i++)
        {
            float x = (i + 0.5f) / n * 2 - 1, y = (j + 0.5f) / n * 2 - 1;
            float r = (float)Math.Sqrt(x * x + y * y);
            int k = (j * n + i) * 4;
            p[k + 0] = 0.50f; p[k + 1] = 0.78f; p[k + 2] = 1.0f;
            p[k + 3] = (float)Math.Pow(C01(1 - r), 2.2);
        }
        Save(p, n, n, path);
    }

    // Preview: each texture composited additively (rgb*alpha) on black, with a red frame border.
    public static void PreviewSheet(string outPath, string[] files)
    {
        int cell = 512, pad = 8;
        var sheet = new Bitmap(cell * files.Length + pad * (files.Length + 1), 128 + 512 + pad * 2, PixelFormat.Format32bppArgb);
        using (var g = Graphics.FromImage(sheet))
        {
            g.Clear(Color.Black);
            for (int f = 0; f < files.Length; f++)
            {
                using (var src = new Bitmap(files[f]))
                {
                    var flat = new Bitmap(src.Width, src.Height, PixelFormat.Format32bppArgb);
                    for (int j = 0; j < src.Height; j++)
                    for (int i = 0; i < src.Width; i++)
                    {
                        Color c = src.GetPixel(i, j);
                        float a = c.A / 255f;
                        flat.SetPixel(i, j, Color.FromArgb(255, (int)(c.R * a), (int)(c.G * a), (int)(c.B * a)));
                    }
                    int dw = src.Width >= src.Height ? cell : cell * src.Width / src.Height;
                    int dh = src.Width >= src.Height ? cell * src.Height / src.Width : cell;
                    int x0 = pad + f * (cell + pad), y0 = pad;
                    g.DrawImage(flat, x0, y0, dw, dh);
                    g.DrawRectangle(Pens.Red, x0, y0, dw - 1, dh - 1);
                    flat.Dispose();
                }
            }
        }
        sheet.Save(outPath, ImageFormat.Png);
        sheet.Dispose();
    }
}
'@

New-Item -ItemType Directory -Force $OutDir | Out-Null
$ray  = Join-Path $OutDir "SF_Ray.png"
$core = Join-Path $OutDir "SF_Core.png"
$glow = Join-Path $OutDir "SF_Glow.png"
$rayV = Join-Path $OutDir "SF_RayV.png"
[StarFlareTex]::Ray($ray, 512, 64)
[StarFlareTex]::RotateToVertical($ray, $rayV)
[StarFlareTex]::Core($core, 128)
[StarFlareTex]::Glow($glow, 128)
"wrote $ray, $rayV, $core, $glow"
if ($Preview -ne "") { [StarFlareTex]::PreviewSheet($Preview, @($ray, $core, $glow)); "preview $Preview" }
