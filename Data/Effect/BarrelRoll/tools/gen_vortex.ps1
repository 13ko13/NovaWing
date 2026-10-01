# BarrelRoll v2 の渦テクスチャ生成
# 使い方: powershell -NoProfile -ExecutionPolicy Bypass -File gen_vortex.ps1 [-OutDir ..\Texture] [-Suffix ""]
#   BR_Swirl_R/L.png : 何本もの弧が渦を巻く円盤（R=反時計回りに進む弧、L=左右反転）
#   BR_BigArc_R/L.png: 外側に飛ぶ大きな弧1本
#   BR_Glow.png      : 中心の閃光
#   preview_vortex.png: 夜空色の背景に合成した確認用
param(
    [string]$OutDir = (Join-Path $PSScriptRoot "..\Texture"),
    [string]$Suffix = ""
)

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class Vortex
{
    static float C01(float x) { return x < 0 ? 0 : (x > 1 ? 1 : x); }

    public class Img
    {
        public int N; public float[] R, G, B, A;
        public Img(int n) { N = n; R = new float[n * n]; G = new float[n * n]; B = new float[n * n]; A = new float[n * n]; }
    }

    static void Save(Img im, string path, bool mirror)
    {
        int n = im.N;
        var bmp = new Bitmap(n, n, PixelFormat.Format32bppArgb);
        var data = bmp.LockBits(new Rectangle(0, 0, n, n), ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
        byte[] buf = new byte[n * n * 4];
        for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++)
            {
                int s = y * n + (mirror ? n - 1 - x : x);
                int d = (y * n + x) * 4;
                buf[d + 0] = (byte)(C01(im.B[s]) * 255);
                buf[d + 1] = (byte)(C01(im.G[s]) * 255);
                buf[d + 2] = (byte)(C01(im.R[s]) * 255);
                buf[d + 3] = (byte)(C01(im.A[s]) * 255);
            }
        Marshal.Copy(buf, 0, data.Scan0, buf.Length);
        bmp.UnlockBits(data);
        bmp.Save(path, ImageFormat.Png);
        bmp.Dispose();
    }

    public static void SavePair(Img im, string dir, string name, string suffix)
    {
        Save(im, System.IO.Path.Combine(dir, name + "_R" + suffix + ".png"), false);
        Save(im, System.IO.Path.Combine(dir, name + "_L" + suffix + ".png"), true);
    }

    public static void SaveOne(Img im, string dir, string name, string suffix)
    {
        Save(im, System.IO.Path.Combine(dir, name + suffix + ".png"), false);
    }

    // 弧を1本描き込む。弧は尾(s=0)から頭(s=1)へ反時計回りに進み、尾ほど内側・細く・薄い。
    // 頭は丸く、芯は白、外側は淡い青。
    static void DrawArc(Img im, float ri, float a0, float span, float w, float inward, float glow, float bright)
    {
        int n = im.N;
        float px = 2.0f / n;
        for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++)
            {
                float fx = (x + 0.5f) * px - 1, fy = 1 - (y + 0.5f) * px;
                float r = (float)Math.Sqrt(fx * fx + fy * fy);
                float th = (float)Math.Atan2(fy, fx);
                float d = th - a0;
                while (d < 0) d += (float)(Math.PI * 2);
                while (d >= Math.PI * 2) d -= (float)(Math.PI * 2);
                float s = d / span;
                if (s > 1.0f + (w * 8 + 0.02f) / (span * ri)) continue;
                float sc = Math.Min(s, 1);
                float rr = ri - inward * (1 - sc);
                float ws = w * (0.12f + 0.88f * (float)Math.Pow(sc, 0.7));
                float dist = Math.Abs(r - rr);
                if (s > 1)
                {
                    // 丸い頭
                    float over = (s - 1) * span * rr;
                    dist = (float)Math.Sqrt(dist * dist + over * over);
                }
                float cov = C01((ws - dist) / px + 0.5f);
                float fade = (float)Math.Pow(sc, 1.3) * bright;
                float core = C01((ws * 0.45f - dist) / px + 0.5f);
                float g = glow * (float)Math.Exp(-dist / (ws * 2.2f + 0.004f)) * fade;
                float a = Math.Max(cov * fade, g);
                int i = y * n + x;
                if (a <= im.A[i]) continue;
                // 芯は白、縁とグローは淡い青
                float wv = cov > 0 ? 0.55f + 0.45f * core : 0.0f;
                im.R[i] = 0.62f + 0.38f * wv;
                im.G[i] = 0.80f + 0.20f * wv;
                im.B[i] = 1.0f;
                im.A[i] = a;
            }
    }

    static void EdgeFade(Img im, float rIn, float rOut)
    {
        int n = im.N;
        float px = 2.0f / n;
        for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++)
            {
                float fx = (x + 0.5f) * px - 1, fy = 1 - (y + 0.5f) * px;
                float r = (float)Math.Sqrt(fx * fx + fy * fy);
                float k = C01((rOut - r) / 0.05f) * C01((r - rIn) / 0.08f);
                im.A[y * n + x] *= k;
            }
    }

    public static Img Swirl(int n, int seed)
    {
        var im = new Img(n);
        var rnd = new Random(seed);
        // 薄い円盤の膜（中は機体が見えるように抜く）
        float px = 2.0f / n;
        for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++)
            {
                float fx = (x + 0.5f) * px - 1, fy = 1 - (y + 0.5f) * px;
                float r = (float)Math.Sqrt(fx * fx + fy * fy);
                float m = 0.14f * C01((r - 0.25f) / 0.25f) * C01((0.93f - r) / 0.25f);
                int i = y * n + x;
                im.R[i] = 0.55f; im.G[i] = 0.75f; im.B[i] = 1.0f; im.A[i] = m;
            }
        int arcs = 16;
        for (int k = 0; k < arcs; k++)
        {
            float u = (k + 0.5f) / arcs;
            float ri = 0.30f + 0.60f * u + (float)(rnd.NextDouble() - 0.5) * 0.03f;
            float a0 = (float)(rnd.NextDouble() * Math.PI * 2);
            float span = 2.0f + (float)rnd.NextDouble() * 2.2f;
            float w = 0.012f + 0.024f * u + (float)rnd.NextDouble() * 0.006f;
            DrawArc(im, ri, a0, span, w, 0.06f, 0.35f, 0.75f + 0.25f * (float)rnd.NextDouble());
        }
        EdgeFade(im, 0.22f, 0.97f);
        return im;
    }

    public static Img BigArc(int n)
    {
        var im = new Img(n);
        DrawArc(im, 0.82f, 0.4f, 3.4f, 0.045f, 0.08f, 0.5f, 1.0f);
        EdgeFade(im, 0.0f, 0.98f);
        return im;
    }

    public static Img Glow(int n)
    {
        var im = new Img(n);
        float px = 2.0f / n;
        for (int y = 0; y < n; y++)
            for (int x = 0; x < n; x++)
            {
                float fx = (x + 0.5f) * px - 1, fy = 1 - (y + 0.5f) * px;
                float r = (float)Math.Sqrt(fx * fx + fy * fy);
                float a = (float)Math.Exp(-r * r / 0.08) * 0.9f + (float)Math.Exp(-r / 0.25) * 0.25f;
                a *= C01((0.98f - r) / 0.2f);
                int i = y * n + x;
                im.R[i] = 0.75f; im.G[i] = 0.88f; im.B[i] = 1.0f; im.A[i] = a;
            }
        return im;
    }

    // 夜空色の背景に合成して並べた確認画像
    public static void Preview(Img[] ims, string path)
    {
        int n = ims[0].N, cnt = ims.Length;
        var bmp = new Bitmap(n * cnt, n, PixelFormat.Format24bppRgb);
        for (int k = 0; k < cnt; k++)
            for (int y = 0; y < n; y++)
                for (int x = 0; x < n; x++)
                {
                    int i = y * n + x;
                    var im = ims[k];
                    float a = C01(im.A[i]);
                    float br = 0.04f, bg = 0.07f, bb = 0.16f;
                    int R = (int)(255 * C01(br + (im.R[i] - br) * a));
                    int G = (int)(255 * C01(bg + (im.G[i] - bg) * a));
                    int B = (int)(255 * C01(bb + (im.B[i] - bb) * a));
                    if (x == 0 || y == 0) { R = 255; G = 0; B = 0; }
                    bmp.SetPixel(k * n + x, y, Color.FromArgb(R, G, B));
                }
        bmp.Save(path, ImageFormat.Png);
        bmp.Dispose();
    }
}
'@

New-Item -ItemType Directory -Force $OutDir | Out-Null
$OutDir = (Resolve-Path $OutDir).Path
$swirl = [Vortex]::Swirl(512, 7)
$big = [Vortex]::BigArc(512)
$glow = [Vortex]::Glow(128)
[Vortex]::SavePair($swirl, $OutDir, "BR_Swirl", $Suffix)
[Vortex]::SavePair($big, $OutDir, "BR_BigArc", $Suffix)
[Vortex]::SaveOne($glow, $OutDir, "BR_Glow", $Suffix)
[Vortex]::Preview(@($swirl, $big), (Join-Path $PSScriptRoot "preview_vortex.png"))
Write-Output "done: $OutDir"
