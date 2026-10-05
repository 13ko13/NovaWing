# Procedural textures for the Boost effect (v1). White RGB, shape in alpha (tint with node colors).
# Usage: powershell -NoProfile -ExecutionPolicy Bypass -File gen_boost_textures.ps1 <outDir> [suffix]
param([string]$OutDir, [string]$Suffix = "")

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class BoostTex
{
    static float C01(float x) { return x < 0 ? 0 : (x > 1 ? 1 : x); }
    static float Smooth(float e0, float e1, float x) { float t = C01((x - e0) / (e1 - e0)); return t * t * (3 - 2 * t); }
    const float TAU = 6.2831853f;

    static void Save(float[] a, int w, int h, string path)
    {
        var bmp = new Bitmap(w, h, PixelFormat.Format32bppArgb);
        var data = bmp.LockBits(new Rectangle(0, 0, w, h), ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
        byte[] buf = new byte[w * h * 4];
        for (int i = 0; i < w * h; i++)
        {
            buf[i * 4 + 0] = 255; buf[i * 4 + 1] = 255; buf[i * 4 + 2] = 255;
            buf[i * 4 + 3] = (byte)(C01(a[i]) * 255);
        }
        Marshal.Copy(buf, 0, data.Scan0, buf.Length);
        bmp.UnlockBits(data);
        bmp.Save(path, ImageFormat.Png);
        bmp.Dispose();
    }

    // Seamless (both directions) streaky flame for a Ring cone with UV scroll.
    // U = around the cone (many thin streaks), V = along the flame (slow variation).
    public static void FlameTile(string path)
    {
        int n = 256; var a = new float[n * n];
        var rnd = new Random(7);
        int K = 7;
        int[] fu = { 5, 8, 11, 13, 17, 23, 29 };
        int[] fv = { 1, -2, 1, 3, -1, 2, -3 };
        float[] amp = { 0.30f, 0.22f, 0.16f, 0.12f, 0.09f, 0.07f, 0.04f };
        float[] ph = new float[K];
        for (int k = 0; k < K; k++) ph[k] = (float)(rnd.NextDouble() * TAU);
        for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++)
            {
                float u = (float)x / n, v = (float)y / n, s = 0;
                for (int k = 0; k < K; k++) s += amp[k] * (float)Math.Sin(TAU * (fu[k] * u + fv[k] * v) + ph[k]);
                float f = C01(0.5f + s);
                a[y * n + x] = 0.25f + 0.75f * (float)Math.Pow(f, 1.6);
            }
        Save(a, n, n, path);
    }

    // Horizontal light streak, zero alpha before the image border (safe to stretch).
    public static void Streak(string path)
    {
        int w = 256, h = 32; var a = new float[w * h];
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                float u = (x + 0.5f) / w * 2 - 1, v = (y + 0.5f) / h * 2 - 1;
                float along = 1 - Smooth(0.15f, 0.9f, Math.Abs(u));
                float head = 0.55f + 0.45f * C01((u + 1) * 0.5f); // slightly brighter at +U end
                float across = (float)Math.Exp(-v * v / 0.06f) * (1 - Smooth(0.8f, 0.95f, Math.Abs(v)));
                a[y * w + x] = along * across * head;
            }
        Save(a, w, h, path);
    }

    // Trail cross-section: soft core across V, constant along U.
    public static void Trail(string path)
    {
        int w = 64, h = 64; var a = new float[w * h];
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
            {
                float v = (y + 0.5f) / h * 2 - 1;
                float core = (float)Math.Exp(-v * v / 0.03f);
                float halo = 0.45f * (float)Math.Exp(-v * v / 0.25f);
                a[y * w + x] = C01(core + halo) * (1 - Smooth(0.85f, 1.0f, Math.Abs(v)));
            }
        Save(a, w, h, path);
    }

    // Round glow, alpha 0 at the border.
    public static void Glow(string path, float tight)
    {
        int n = 128; var a = new float[n * n];
        for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++)
            {
                float u = (x + 0.5f) / n * 2 - 1, v = (y + 0.5f) / n * 2 - 1;
                float r = (float)Math.Sqrt(u * u + v * v);
                a[y * n + x] = (float)Math.Exp(-r * r * tight) * (1 - Smooth(0.7f, 1.0f, r));
            }
        Save(a, n, n, path);
    }

    // Soft noisy puff for exhaust / air.
    public static void Puff(string path, int seed)
    {
        int n = 128; var a = new float[n * n];
        var rnd = new Random(seed);
        int K = 6; float[] fx = new float[K], fy = new float[K], ph = new float[K];
        for (int k = 0; k < K; k++) { fx[k] = 1 + rnd.Next(4); fy[k] = 1 + rnd.Next(4); ph[k] = (float)(rnd.NextDouble() * TAU); }
        for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++)
            {
                float u = (x + 0.5f) / n * 2 - 1, v = (y + 0.5f) / n * 2 - 1;
                float r = (float)Math.Sqrt(u * u + v * v), s = 0;
                for (int k = 0; k < K; k++) s += (float)Math.Sin(fx[k] * u * 3.1f + fy[k] * v * 2.7f + ph[k]) / K;
                float body = (float)Math.Exp(-r * r * 3.0f);
                a[y * n + x] = C01(body * (0.7f + 0.6f * s)) * (1 - Smooth(0.6f, 1.0f, r));
            }
        Save(a, n, n, path);
    }

    // Thin ring for the ignition shock wave.
    public static void ShockRing(string path)
    {
        int n = 256; var a = new float[n * n];
        for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++)
            {
                float u = (x + 0.5f) / n * 2 - 1, v = (y + 0.5f) / n * 2 - 1;
                float r = (float)Math.Sqrt(u * u + v * v);
                float d = r - 0.78f;
                float ring = d > 0 ? (float)Math.Exp(-d * d / 0.0012f) : (float)Math.Exp(-d * d / 0.012f);
                a[y * n + x] = ring * (1 - Smooth(0.9f, 0.99f, r));
            }
        Save(a, n, n, path);
    }
}
'@

New-Item -ItemType Directory -Force $OutDir | Out-Null
[BoostTex]::FlameTile((Join-Path $OutDir "BS_FlameTile$Suffix.png"))
[BoostTex]::Streak((Join-Path $OutDir "BS_Streak$Suffix.png"))
[BoostTex]::Trail((Join-Path $OutDir "BS_Trail$Suffix.png"))
[BoostTex]::Glow((Join-Path $OutDir "BS_Glow$Suffix.png"), 4.0)
[BoostTex]::Puff((Join-Path $OutDir "BS_Puff$Suffix.png"), 3)
[BoostTex]::ShockRing((Join-Path $OutDir "BS_ShockRing$Suffix.png"))
Write-Output "done: $OutDir"
