# Procedural textures shared by Charging (charge-up) and PlayerChargeBullet (charge shot).
# Both effects use the same set so they read as one family.
# Usage: powershell -NoProfile -ExecutionPolicy Bypass -File gen_charge_tex.ps1 [outDir] [suffix]
param(
    [string]$OutDir = (Join-Path (Split-Path -Parent $PSScriptRoot) "Texture"),
    [string]$Suffix = ""
)

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class CsTex
{
    static float C01(float x) { return x < 0 ? 0 : (x > 1 ? 1 : x); }
    static float Lerp(float a, float b, float t) { return a + (b - a) * t; }
    static float Smooth(float e0, float e1, float x) { float t = C01((x - e0) / (e1 - e0)); return t * t * (3 - 2 * t); }
    const float TAU = (float)(Math.PI * 2);

    // palette shared with PlayerBullet (gen_bullet_textures.ps1)
    static readonly float[] CORE = { 235 / 255f, 1f, 238 / 255f };
    static readonly float[] GLOW = { 30 / 255f, 1f, 110 / 255f };

    static void Save(float[] r, float[] g, float[] b, float[] a, int w, int h, string path)
    {
        var bmp = new Bitmap(w, h, PixelFormat.Format32bppArgb);
        var data = bmp.LockBits(new Rectangle(0, 0, w, h), ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
        byte[] buf = new byte[w * h * 4];
        for (int i = 0; i < w * h; i++)
        {
            buf[i * 4 + 0] = (byte)(C01(b[i]) * 255 + 0.5f);
            buf[i * 4 + 1] = (byte)(C01(g[i]) * 255 + 0.5f);
            buf[i * 4 + 2] = (byte)(C01(r[i]) * 255 + 0.5f);
            buf[i * 4 + 3] = (byte)(C01(a[i]) * 255 + 0.5f);
        }
        Marshal.Copy(buf, 0, data.Scan0, buf.Length);
        bmp.UnlockBits(data);
        bmp.Save(path, ImageFormat.Png);
        bmp.Dispose();
    }

    // ---------- 3D value noise ----------
    static float Hash(int x, int y, int z, int seed)
    {
        unchecked
        {
            int h = x * 374761393 + y * 668265263 + z * 2147483647 + seed * 144665;
            h = (h ^ (h >> 13)) * 1274126177;
            h = h ^ (h >> 16);
            return (h & 0x7fffffff) / (float)0x7fffffff;
        }
    }
    static float Fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
    static float Noise(float x, float y, float z, int seed)
    {
        int ix = (int)Math.Floor(x), iy = (int)Math.Floor(y), iz = (int)Math.Floor(z);
        float fx = Fade(x - ix), fy = Fade(y - iy), fz = Fade(z - iz);
        float c000 = Hash(ix, iy, iz, seed), c100 = Hash(ix + 1, iy, iz, seed);
        float c010 = Hash(ix, iy + 1, iz, seed), c110 = Hash(ix + 1, iy + 1, iz, seed);
        float c001 = Hash(ix, iy, iz + 1, seed), c101 = Hash(ix + 1, iy, iz + 1, seed);
        float c011 = Hash(ix, iy + 1, iz + 1, seed), c111 = Hash(ix + 1, iy + 1, iz + 1, seed);
        float x00 = Lerp(c000, c100, fx), x10 = Lerp(c010, c110, fx);
        float x01 = Lerp(c001, c101, fx), x11 = Lerp(c011, c111, fx);
        return Lerp(Lerp(x00, x10, fy), Lerp(x01, x11, fy), fz);
    }
    static float Fbm(float x, float y, float z, int seed, int oct)
    {
        float s = 0, amp = 0.5f, norm = 0, f = 1;
        for (int i = 0; i < oct; i++)
        {
            s += Noise(x * f, y * f, z * f, seed + i * 17) * amp; norm += amp; amp *= 0.5f; f *= 2.03f;
        }
        return s / norm;
    }

    // green ramp: 0 = deep emerald, 0.6 = player-bullet green, 1 = white-green core
    static void Ramp(float t, out float r, out float g, out float b)
    {
        t = C01(t);
        float[] deep = { 0.02f, 0.45f, 0.22f };
        if (t < 0.6f)
        {
            float u = t / 0.6f;
            r = Lerp(deep[0], GLOW[0], u); g = Lerp(deep[1], GLOW[1], u); b = Lerp(deep[2], GLOW[2], u);
        }
        else
        {
            float u = (t - 0.6f) / 0.4f;
            r = Lerp(GLOW[0], CORE[0], u); g = Lerp(GLOW[1], CORE[1], u); b = Lerp(GLOW[2], CORE[2], u);
        }
    }

    // ---------- Plasma wisps: swirling spiral tendrils (the "look" of the reference) ----------
    // A loop of cols*rows frames. Each frame holds `arms` spiral tendrils around the center,
    // domain-warped by noise that travels on a circle (so the loop is seamless).
    // Tendrils have a brighter rim and a see-through body, like curling plasma smoke.
    public static void WispSheet(string path, int cols, int rows, int cell, int arms, int seed)
    {
        int W = cols * cell, H = rows * cell, frames = cols * rows;
        float[] R = new float[W * H], G = new float[W * H], B = new float[W * H], A = new float[W * H];
        var rnd = new Random(seed);
        float[] a0 = new float[arms], span = new float[arms], rIn = new float[arms], rOut = new float[arms], wid = new float[arms];
        for (int k = 0; k < arms; k++)
        {
            a0[k] = (k + (float)rnd.NextDouble() * 0.5f) / arms * TAU;
            span[k] = 2.0f + (float)rnd.NextDouble() * 1.6f;   // radians the arm sweeps
            rIn[k] = (arms == 1 ? 0.30f : 0.12f) + (float)rnd.NextDouble() * 0.12f;
            rOut[k] = 0.62f + (float)rnd.NextDouble() * 0.18f;
            wid[k] = (arms == 1 ? 0.10f : 0.075f) + (float)rnd.NextDouble() * 0.04f;
        }
        for (int f = 0; f < frames; f++)
        {
            float t = f / (float)frames;
            float cx = 0.55f * (float)Math.Cos(TAU * t), cy = 0.55f * (float)Math.Sin(TAU * t);
            int ox = (f % cols) * cell, oy = (f / cols) * cell;
            for (int py = 0; py < cell; py++)
                for (int px = 0; px < cell; px++)
                {
                    float x = (px + 0.5f) / cell * 2 - 1, y = -((py + 0.5f) / cell * 2 - 1);
                    // turbulent warp
                    float wx = Fbm(x * 2.2f + cx, y * 2.2f + cy, 1.3f, seed + 3, 4) - 0.5f;
                    float wy = Fbm(x * 2.2f + cx + 5.3f, y * 2.2f + cy, 2.9f, seed + 7, 4) - 0.5f;
                    float hx = Fbm(x * 6f + cx, y * 6f + cy, 8.7f, seed + 19, 3) - 0.5f;
                    float hy = Fbm(x * 6f + cx + 3.1f, y * 6f + cy, 6.2f, seed + 23, 3) - 0.5f;
                    float xx = x + wx * 0.42f + hx * 0.10f, yy = y + wy * 0.42f + hy * 0.10f;
                    float rr = (float)Math.Sqrt(xx * xx + yy * yy);
                    float ang = (float)Math.Atan2(yy, xx);
                    float dens = 0, rim = 0;
                    for (int k = 0; k < arms; k++)
                    {
                        // the arm's angle at this radius (spiral): angle grows as radius grows
                        float s = C01((rr - rIn[k]) / (rOut[k] - rIn[k]));       // 0 inner end .. 1 outer end
                        float armAng = a0[k] + span[k] * s + TAU * t * 0.0f;
                        float da = ang - armAng;
                        while (da > Math.PI) da -= TAU;
                        while (da < -Math.PI) da += TAU;
                        float across = Math.Abs(da) * Math.Max(rr, 0.08f);
                        float w = wid[k] * (0.35f + 1.1f * (float)Math.Sin(Math.PI * Math.Pow(s, 0.8))) ;
                        float inR = Smooth(rIn[k] - 0.06f, rIn[k] + 0.04f, rr) * (1 - Smooth(rOut[k] - 0.08f, rOut[k] + 0.04f, rr));
                        float q = across / Math.Max(w, 0.01f);                  // 0 centre line, 1 edge
                        float body = (1 - Smooth(0.55f, 1.0f, q)) * inR;
                        float edge = (float)Math.Exp(-Math.Pow((q - 0.78f) / 0.16f, 2)) * inR;
                        dens = Math.Max(dens, body);
                        rim = Math.Max(rim, edge);
                    }
                    // fine filaments break the bands into smoky strands
                    float fil = Fbm(x * 7f + cx * 2, y * 7f + cy * 2, 4.1f, seed + 13, 3);
                    float ridge = 1 - Math.Abs(Fbm(x * 9f + cx * 3, y * 9f + cy * 3, 2.3f, seed + 31, 3) * 2 - 1);
                    float strands = Smooth(0.30f, 0.62f, fil) * 0.5f + (float)Math.Pow(ridge, 4) * 0.7f;
                    float v = dens * (0.12f + 0.55f * strands) + rim * (0.35f + 0.65f * strands);
                    v *= 1 - Smooth(0.80f, 0.97f, (float)Math.Sqrt(x * x + y * y));   // keep inside the cell
                    float r, g, b;
                    Ramp(0.35f + 0.65f * C01(rim * 0.9f + dens * 0.25f), out r, out g, out b);
                    int i = (oy + py) * W + ox + px;
                    R[i] = r; G[i] = g; B[i] = b; A[i] = C01(v * 1.15f);
                }
        }
        Save(R, G, B, A, W, H, path);
    }

    // ---------- Core: white-green hot centre with a green halo, colour baked in ----------
    public static void Core(string path, int s)
    {
        float[] R = new float[s * s], G = new float[s * s], B = new float[s * s], A = new float[s * s];
        for (int py = 0; py < s; py++)
            for (int px = 0; px < s; px++)
            {
                float x = (px + 0.5f) / s * 2 - 1, y = (py + 0.5f) / s * 2 - 1;
                float r = (float)Math.Sqrt(x * x + y * y);
                float hot = (float)Math.Exp(-r * r / 0.025f);
                float halo = (float)Math.Pow(C01(1 - r), 2.4f);
                float rr, gg, bb;
                Ramp(0.55f + 0.45f * hot, out rr, out gg, out bb);
                int i = py * s + px;
                R[i] = rr; G[i] = gg; B[i] = bb; A[i] = C01(hot + halo * 0.75f) * (1 - Smooth(0.92f, 0.99f, r));
            }
        Save(R, G, B, A, s, s, path);
    }

    // ---------- Soft glow (white, tinted by the node) ----------
    public static void Glow(string path, int s, float power)
    {
        float[] R = new float[s * s], G = new float[s * s], B = new float[s * s], A = new float[s * s];
        for (int py = 0; py < s; py++)
            for (int px = 0; px < s; px++)
            {
                float x = (px + 0.5f) / s * 2 - 1, y = (py + 0.5f) / s * 2 - 1;
                float r = (float)Math.Sqrt(x * x + y * y);
                int i = py * s + px;
                R[i] = G[i] = B[i] = 1; A[i] = (float)Math.Pow(C01(1 - r), power);
            }
        Save(R, G, B, A, s, s, path);
    }

    // ---------- Sparkle: 4 long + 4 short sharp rays (lens-star) ----------
    public static void Star(string path, int s)
    {
        float[] R = new float[s * s], G = new float[s * s], B = new float[s * s], A = new float[s * s];
        for (int py = 0; py < s; py++)
            for (int px = 0; px < s; px++)
            {
                float x = (px + 0.5f) / s * 2 - 1, y = (py + 0.5f) / s * 2 - 1;
                float r = (float)Math.Sqrt(x * x + y * y);
                float ang = (float)Math.Atan2(y, x);
                float v = (float)Math.Exp(-r * r / 0.004f) + (float)Math.Exp(-r * r / 0.03f) * 0.35f;
                for (int k = 0; k < 8; k++)
                {
                    float ra = k * TAU / 8;
                    bool longRay = (k % 2) == 0;
                    float len = longRay ? 0.95f : 0.45f;
                    float da = ang - ra;
                    while (da > Math.PI) da -= TAU;
                    while (da < -Math.PI) da += TAU;
                    if (Math.Abs(da) > 1.4f) continue;
                    float across = Math.Abs((float)Math.Sin(da)) * r;
                    float along = C01(1 - r / len);
                    float w = (longRay ? 0.026f : 0.018f) * (0.3f + along);
                    v += (float)Math.Exp(-across * across / (w * w)) * along * along * (longRay ? 1f : 0.6f);
                }
                v *= 1 - Smooth(0.92f, 0.99f, r);
                int i = py * s + px;
                R[i] = 1; G[i] = 1; B[i] = 1; A[i] = C01(v);
            }
        Save(R, G, B, A, s, s, path);
    }

    // ---------- Arcs: short lightning crawling around the orb (cols x rows variants) ----------
    static void Bolt(float[] pts, int n, Random rnd, int lo, int hi, float disp)
    {
        if (hi - lo < 2) return;
        int mid = (lo + hi) / 2;
        float ax = pts[lo * 2], ay = pts[lo * 2 + 1], bx = pts[hi * 2], by = pts[hi * 2 + 1];
        float dx = bx - ax, dy = by - ay;
        float len = (float)Math.Sqrt(dx * dx + dy * dy);
        float off = ((float)rnd.NextDouble() * 2 - 1) * disp * len;
        pts[mid * 2] = (ax + bx) / 2 - dy / len * off;
        pts[mid * 2 + 1] = (ay + by) / 2 + dx / len * off;
        Bolt(pts, n, rnd, lo, mid, disp);
        Bolt(pts, n, rnd, mid, hi, disp);
    }
    static float SegDist(float px, float py, float ax, float ay, float bx, float by)
    {
        float dx = bx - ax, dy = by - ay;
        float t = C01(((px - ax) * dx + (py - ay) * dy) / (dx * dx + dy * dy + 1e-9f));
        float qx = ax + dx * t - px, qy = ay + dy * t - py;
        return (float)Math.Sqrt(qx * qx + qy * qy);
    }
    public static void ArcSheet(string path, int cols, int rows, int cell, int seed)
    {
        int W = cols * cell, H = rows * cell;
        float[] R = new float[W * H], G = new float[W * H], B = new float[W * H], A = new float[W * H];
        const int N = 33;
        for (int v = 0; v < cols * rows; v++)
        {
            var rnd = new Random(seed + v * 101);
            // endpoints on a circle of radius ~0.55: the arc hugs the orb's surface
            float a1 = (float)rnd.NextDouble() * TAU, a2 = a1 + 1.4f + (float)rnd.NextDouble() * 1.4f;
            float rad = 0.42f + (float)rnd.NextDouble() * 0.1f;
            float[] pts = new float[N * 2];
            // seed the polyline along the circular arc, then displace
            for (int k = 0; k < N; k++)
            {
                float u = k / (float)(N - 1);
                float a = Lerp(a1, a2, u);
                float rr = rad * (1 + 0.15f * (float)Math.Sin(Math.PI * u));
                pts[k * 2] = (float)Math.Cos(a) * rr; pts[k * 2 + 1] = (float)Math.Sin(a) * rr;
            }
            float[] bolt = (float[])pts.Clone();
            for (int k = 1; k < N - 1; k++) { bolt[k * 2] += 0; }
            // midpoint displacement on top of the arc (fixed ratio keeps it jagged)
            float[] tmp = new float[N * 2];
            tmp[0] = bolt[0]; tmp[1] = bolt[1]; tmp[(N - 1) * 2] = bolt[(N - 1) * 2]; tmp[(N - 1) * 2 + 1] = bolt[(N - 1) * 2 + 1];
            Bolt(tmp, N, rnd, 0, N - 1, 0.30f);
            for (int k = 0; k < N; k++)
            {
                // blend displaced straight bolt with the circular guide: keeps the curve, adds jaggies
                float u = k / (float)(N - 1);
                float sx = Lerp(bolt[0], bolt[(N - 1) * 2], u), sy = Lerp(bolt[1], bolt[(N - 1) * 2 + 1], u);
                bolt[k * 2] = pts[k * 2] + (tmp[k * 2] - sx) * 1.1f;
                bolt[k * 2 + 1] = pts[k * 2 + 1] + (tmp[k * 2 + 1] - sy) * 1.1f;
            }
            int ox = (v % cols) * cell, oy = (v / cols) * cell;
            for (int py = 0; py < cell; py++)
                for (int px = 0; px < cell; px++)
                {
                    float x = (px + 0.5f) / cell * 2 - 1, y = (py + 0.5f) / cell * 2 - 1;
                    float d = 9;
                    float taper = 1;
                    for (int k = 0; k < N - 1; k++)
                    {
                        float dd = SegDist(x, y, bolt[k * 2], bolt[k * 2 + 1], bolt[k * 2 + 2], bolt[k * 2 + 3]);
                        if (dd < d) { d = dd; taper = (float)Math.Sin(Math.PI * (k + 0.5f) / (N - 1)); }
                    }
                    float core = (float)Math.Exp(-d * d / (0.006f * 0.006f * (0.3f + taper)));
                    float glow = (float)Math.Exp(-d / 0.05f) * 0.45f * taper;
                    int i = (oy + py) * W + ox + px;
                    float c = C01(core);
                    R[i] = Lerp(GLOW[0] * 0.6f + 0.4f, 1, c); G[i] = 1; B[i] = Lerp(GLOW[2] * 0.6f + 0.4f, 1, c);
                    A[i] = C01(core + glow) * (1 - Smooth(0.9f, 0.99f, (float)Math.Sqrt(x * x + y * y)));
                }
        }
        Save(R, G, B, A, W, H, path);
    }

    // ---------- Ring: thin bright edge + soft inner, broken by noise ----------
    public static void Ring(string path, int s, int seed)
    {
        float[] R = new float[s * s], G = new float[s * s], B = new float[s * s], A = new float[s * s];
        for (int py = 0; py < s; py++)
            for (int px = 0; px < s; px++)
            {
                float x = (px + 0.5f) / s * 2 - 1, y = (py + 0.5f) / s * 2 - 1;
                float r = (float)Math.Sqrt(x * x + y * y);
                float ang = (float)Math.Atan2(y, x);
                float n = Fbm((float)Math.Cos(ang) * 3f, (float)Math.Sin(ang) * 3f, 0.3f, seed, 4);
                float edge = (float)Math.Exp(-Math.Pow((r - 0.86f) / 0.03f, 2));
                float inner = Smooth(0.55f, 0.86f, r) * (1 - Smooth(0.86f, 0.9f, r)) * 0.3f;
                float v = (edge + inner) * (0.5f + 0.7f * n);
                v *= 1 - Smooth(0.93f, 0.99f, r);
                int i = py * s + px;
                R[i] = 1; G[i] = 1; B[i] = 1; A[i] = C01(v);
            }
        Save(R, G, B, A, s, s, path);
    }

    // ---------- Spark streak (vertical, head at top) for directional billboards ----------
    public static void Spark(string path, int w, int h)
    {
        float[] R = new float[w * h], G = new float[w * h], B = new float[w * h], A = new float[w * h];
        for (int py = 0; py < h; py++)
            for (int px = 0; px < w; px++)
            {
                float x = (px + 0.5f) / w * 2 - 1, y = (py + 0.5f) / h * 2 - 1;
                float along = C01(1 - Math.Abs(y));
                float head = (float)Math.Exp(-Math.Pow((y + 0.55f) / 0.35f, 2));
                float width = 0.18f + 0.5f * head;
                float across = (float)Math.Exp(-x * x / (width * width * 0.25f));
                float v = across * (float)Math.Pow(along, 0.7f) * (0.45f + 0.55f * head);
                int i = py * w + px;
                R[i] = 1; G[i] = 1; B[i] = 1; A[i] = C01(v * 1.4f);
            }
        Save(R, G, B, A, w, h, path);
    }

    // preview: composite onto a dark game-like background with cell grid
    public static void Preview(string src, string dst, int cols, int rows)
    {
        using (var img = new Bitmap(src))
        using (var bmp = new Bitmap(img.Width, img.Height, PixelFormat.Format32bppArgb))
        using (var g = Graphics.FromImage(bmp))
        {
            g.Clear(Color.FromArgb(255, 18, 22, 34));
            g.DrawImage(img, 0, 0, img.Width, img.Height);
            using (var pen = new Pen(Color.FromArgb(255, 200, 60, 60)))
            {
                for (int c = 1; c < cols; c++) g.DrawLine(pen, c * img.Width / cols, 0, c * img.Width / cols, img.Height);
                for (int r = 1; r < rows; r++) g.DrawLine(pen, 0, r * img.Height / rows, img.Width, r * img.Height / rows);
            }
            bmp.Save(dst, ImageFormat.Png);
        }
    }
}
'@

New-Item -ItemType Directory -Force $OutDir | Out-Null
$review = Join-Path $PSScriptRoot "review"
New-Item -ItemType Directory -Force $review | Out-Null

function P($name) { Join-Path $OutDir ("CS_" + $name + $Suffix + ".png") }

[CsTex]::WispSheet((P "Wisp"), 4, 4, 256, 1, 11)
[CsTex]::WispSheet((P "Wisp2"), 4, 4, 256, 2, 37)
[CsTex]::Core((P "Core"), 128)
[CsTex]::Glow((P "Glow"), 128, 2.2)
[CsTex]::Star((P "Star"), 256)
[CsTex]::ArcSheet((P "Arc"), 2, 2, 256, 5)
[CsTex]::Ring((P "Ring"), 256, 3)
[CsTex]::Spark((P "Spark"), 32, 128)

[CsTex]::Preview((P "Wisp"), (Join-Path $review "tex_wisp.png"), 4, 4)
[CsTex]::Preview((P "Wisp2"), (Join-Path $review "tex_wisp2.png"), 4, 4)
[CsTex]::Preview((P "Arc"), (Join-Path $review "tex_arc.png"), 2, 2)
[CsTex]::Preview((P "Star"), (Join-Path $review "tex_star.png"), 1, 1)
[CsTex]::Preview((P "Core"), (Join-Path $review "tex_core.png"), 1, 1)
[CsTex]::Preview((P "Ring"), (Join-Path $review "tex_ring.png"), 1, 1)
"done"
