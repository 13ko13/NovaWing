# Procedural textures for the enemy death explosion (EnemyDeath).
# Usage: powershell -NoProfile -ExecutionPolicy Bypass -File gen_explosion_tex.ps1 [outDir] [suffix]
param(
    [string]$OutDir = (Join-Path (Split-Path -Parent $PSScriptRoot) "Texture"),
    [string]$Suffix = ""
)

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class ExTex
{
    static float C01(float x) { return x < 0 ? 0 : (x > 1 ? 1 : x); }
    static float Lerp(float a, float b, float t) { return a + (b - a) * t; }
    static float Smooth(float e0, float e1, float x) { float t = C01((x - e0) / (e1 - e0)); return t * t * (3 - 2 * t); }

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
    // fbm in [0,1]
    static float Fbm(float x, float y, float z, int seed, int oct)
    {
        float s = 0, amp = 0.5f, norm = 0, f = 1;
        for (int i = 0; i < oct; i++)
        {
            s += Noise(x * f, y * f, z * f, seed + i * 17) * amp; norm += amp; amp *= 0.5f; f *= 2.03f;
        }
        return s / norm;
    }

    // ---------- Puff field: smooth union of spherical billows ----------
    struct Billow { public float x, y, r; }
    static Billow[] MakeBillows(Random rnd, int n, float spread, float rmin, float rmax)
    {
        var b = new Billow[n];
        for (int i = 0; i < n; i++)
        {
            double ang = rnd.NextDouble() * Math.PI * 2;
            double d = Math.Sqrt(rnd.NextDouble()) * spread;
            b[i].x = (float)(Math.Cos(ang) * d);
            b[i].y = (float)(Math.Sin(ang) * d);
            b[i].r = (float)(rmin + rnd.NextDouble() * (rmax - rmin));
        }
        b[0].x = 0; b[0].y = 0; b[0].r = rmax;   // a big one in the middle
        return b;
    }
    // >0 inside, sphere-like profile, smooth max between billows (no hard creases)
    static float BillowField(Billow[] bs, float grow, float x, float y)
    {
        const float k = 15f;
        double sum = 0;
        for (int i = 0; i < bs.Length; i++)
        {
            float r = bs[i].r * grow;
            float dx = x - bs[i].x * grow, dy = y - bs[i].y * grow;
            float s = 1 - (dx * dx + dy * dy) / (r * r);
            float h = s > 0 ? (float)Math.Sqrt(s) : s;
            sum += Math.Exp(k * h);
        }
        return (float)(Math.Log(sum) / k);
    }

    // Blackbody-ish ramp, T in [0,1]
    static void Heat(float T, out float r, out float g, out float b)
    {
        float[] ts = { 0f, 0.18f, 0.40f, 0.65f, 0.88f, 1f };
        float[,] cs = {
            { 0.13f, 0.11f, 0.10f },
            { 0.50f, 0.10f, 0.03f },
            { 0.95f, 0.36f, 0.06f },
            { 1.00f, 0.66f, 0.18f },
            { 1.00f, 0.90f, 0.58f },
            { 1.00f, 1.00f, 0.92f } };
        T = C01(T);
        int k = 0;
        while (k < ts.Length - 2 && T > ts[k + 1]) k++;
        float u = (T - ts[k]) / (ts[k + 1] - ts[k]);
        r = Lerp(cs[k, 0], cs[k + 1, 0], u);
        g = Lerp(cs[k, 1], cs[k + 1, 1], u);
        b = Lerp(cs[k, 2], cs[k + 1, 2], u);
    }

    // full puff density at a point: warped billows + fine noise, masked to stay in the cell
    static float PuffDensity(Billow[] bs, float grow, float x, float y, float z, float rise, float detail, int seed)
    {
        float wx = Fbm(x * 2.0f, y * 2.0f, z, seed + 3, 3) - 0.5f;
        float wy = Fbm(x * 2.0f + 7.1f, y * 2.0f, z, seed + 5, 3) - 0.5f;
        float d = BillowField(bs, grow, x + wx * 0.3f, y + wy * 0.3f - rise);
        d += (Fbm(x * 3.2f, y * 3.2f, z * 1.7f, seed + 11, 4) - 0.5f) * detail;
        float rr = (float)Math.Sqrt(x * x + y * y);
        return d - Smooth(0.84f, 0.98f, rr) * 1.5f;
    }

    // light from upper left (y is up), returns 0..1
    static float Light(float gx, float gy, float bump)
    {
        float nx = -gx * bump, ny = -gy * bump, nz = 1;
        float inv = 1f / (float)Math.Sqrt(nx * nx + ny * ny + nz * nz);
        return C01((nx * -0.45f + ny * 0.7f + nz * 0.55f) * inv);
    }

    // ---------- Fireball flipbook: hot dense puff -> cooling -> eroded smoke ----------
    public static void FireSheet(string path, int cols, int rows, int cell, int seed)
    {
        int W = cols * cell, H = rows * cell, frames = cols * rows;
        float[] R = new float[W * H], G = new float[W * H], B = new float[W * H], A = new float[W * H];
        var rnd = new Random(seed);
        var bs = MakeBillows(rnd, 14, 0.50f, 0.20f, 0.40f);
        float e = 3f / cell;
        for (int f = 0; f < frames; f++)
        {
            float t = f / (float)(frames - 1);
            float grow = 0.82f + 0.18f * (1 - (1 - t) * (1 - t));
            float heat = 1.0f * (1 - Smooth(0f, 0.75f, t));
            float erode = Smooth(0.35f, 1.0f, t) * 0.95f;
            float soft = 0.10f + 0.25f * t;
            float z = t * 1.6f, rise = t * 0.07f;
            int ox = (f % cols) * cell, oy = (f / cols) * cell;
            for (int py = 0; py < cell; py++)
                for (int px = 0; px < cell; px++)
                {
                    float x = (px + 0.5f) / cell * 2 - 1, y = -((py + 0.5f) / cell * 2 - 1);
                    float d = PuffDensity(bs, grow, x, y, z, rise, 0.40f, seed);
                    // erosion eats holes through the puff (noise-driven) instead of just shrinking it
                    float en = Fbm(x * 2.6f, y * 2.6f + t * 0.5f, 5.5f + t * 0.6f, seed + 29, 4);
                    float de = d * 0.75f + en * 0.55f - 0.18f;
                    int i = (oy + py) * W + ox + px;
                    if (d < -0.16f) { R[i] = G[i] = B[i] = 0; A[i] = 0; continue; }
                    float gx = (PuffDensity(bs, grow, x + e, y, z, rise, 0.40f, seed) - PuffDensity(bs, grow, x - e, y, z, rise, 0.40f, seed)) / (2 * e);
                    float gy = (PuffDensity(bs, grow, x, y + e, z, rise, 0.40f, seed) - PuffDensity(bs, grow, x, y - e, z, rise, 0.40f, seed)) / (2 * e);
                    float lit = Light(gx, gy, 0.14f);
                    float cav = Smooth(-0.05f, 0.5f, d);               // crevices / thin parts are cooler
                    float T = (d * 1.1f + 0.10f) * heat * (0.50f + 0.50f * lit) * (0.45f + 0.55f * cav);
                    float r, g, b;
                    Heat(T, out r, out g, out b);
                    float selfLit = Smooth(0.25f, 0.7f, T);
                    float shade = Lerp(0.42f + 0.68f * lit, 1f, selfLit * 0.7f);
                    R[i] = r * shade; G[i] = g * shade; B[i] = b * shade;
                    A[i] = Smooth(-0.14f, 0.24f, d) * Smooth(erode - soft * 0.5f, erode + soft, de + 0.25f);
                }
        }
        Save(R, G, B, A, W, H, path);
    }

    // ---------- Smoke puffs: cols x rows static variants, grey, top-lit ----------
    public static void SmokeSheet(string path, int cols, int rows, int cell, int seed)
    {
        int W = cols * cell, H = rows * cell;
        float[] R = new float[W * H], G = new float[W * H], B = new float[W * H], A = new float[W * H];
        float e = 3f / cell;
        for (int v = 0; v < cols * rows; v++)
        {
            var rnd = new Random(seed + v * 31);
            var bs = MakeBillows(rnd, 12, 0.48f, 0.20f, 0.38f);
            float z = v * 3.3f;
            int s = seed + v;
            int ox = (v % cols) * cell, oy = (v / cols) * cell;
            for (int py = 0; py < cell; py++)
                for (int px = 0; px < cell; px++)
                {
                    float x = (px + 0.5f) / cell * 2 - 1, y = -((py + 0.5f) / cell * 2 - 1);
                    float d = PuffDensity(bs, 1f, x, y, z, 0, 0.5f, s);
                    int i = (oy + py) * W + ox + px;
                    if (d < -0.05f) { R[i] = G[i] = B[i] = 0; A[i] = 0; continue; }
                    float gx = (PuffDensity(bs, 1f, x + e, y, z, 0, 0.5f, s) - PuffDensity(bs, 1f, x - e, y, z, 0, 0.5f, s)) / (2 * e);
                    float gy = (PuffDensity(bs, 1f, x, y + e, z, 0, 0.5f, s) - PuffDensity(bs, 1f, x, y - e, z, 0, 0.5f, s)) / (2 * e);
                    float lit = Light(gx, gy, 0.12f);
                    float c = 0.30f + 0.70f * lit;
                    R[i] = c; G[i] = c * 0.97f; B[i] = c * 0.94f;
                    A[i] = Smooth(-0.10f, 0.40f, d) * 0.9f;
                }
        }
        Save(R, G, B, A, W, H, path);
    }

    // ---------- Simple radial textures ----------
    public static void Glow(string path, int s, float power)
    {
        float[] R = new float[s * s], G = new float[s * s], B = new float[s * s], A = new float[s * s];
        for (int py = 0; py < s; py++)
            for (int px = 0; px < s; px++)
            {
                float x = (px + 0.5f) / s * 2 - 1, y = (py + 0.5f) / s * 2 - 1;
                float r = (float)Math.Sqrt(x * x + y * y);
                float v = (float)Math.Pow(C01(1 - r), power);
                int i = py * s + px;
                R[i] = G[i] = B[i] = 1; A[i] = v;
            }
        Save(R, G, B, A, s, s, path);
    }

    // white-hot flash with thin rays
    public static void Flash(string path, int s, int rays, int seed)
    {
        var rnd = new Random(seed);
        float[] rayAng = new float[rays], rayLen = new float[rays], rayW = new float[rays];
        for (int k = 0; k < rays; k++)
        {
            rayAng[k] = (float)((k + rnd.NextDouble() * 0.6) / rays * Math.PI * 2);
            rayLen[k] = 0.55f + (float)rnd.NextDouble() * 0.4f;
            rayW[k] = 0.010f + (float)rnd.NextDouble() * 0.012f;
        }
        float[] R = new float[s * s], G = new float[s * s], B = new float[s * s], A = new float[s * s];
        for (int py = 0; py < s; py++)
            for (int px = 0; px < s; px++)
            {
                float x = (px + 0.5f) / s * 2 - 1, y = (py + 0.5f) / s * 2 - 1;
                float r = (float)Math.Sqrt(x * x + y * y);
                float core = (float)Math.Exp(-r * r / 0.012f);
                float halo = (float)Math.Pow(C01(1 - r / 0.95f), 3) * 0.55f;
                float ray = 0;
                float ang = (float)Math.Atan2(y, x);
                for (int k = 0; k < rays; k++)
                {
                    float da = ang - rayAng[k];
                    while (da > Math.PI) da -= (float)(Math.PI * 2);
                    while (da < -Math.PI) da += (float)(Math.PI * 2);
                    float across = Math.Abs(da) * r;
                    float along = C01(1 - r / rayLen[k]);
                    ray += (float)Math.Exp(-across * across / (rayW[k] * rayW[k] * (0.3f + along))) * along * along;
                }
                float v = C01(core + halo + ray * 0.8f);
                v *= 1 - Smooth(0.9f, 0.99f, r);
                int i = py * s + px;
                R[i] = 1; G[i] = 0.97f; B[i] = 0.9f; A[i] = v;
            }
        Save(R, G, B, A, s, s, path);
    }

    // shockwave ring: bright thin outer edge, soft inner trail, slightly broken by noise
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
                float edge = (float)Math.Exp(-Math.Pow((r - 0.86f) / 0.035f, 2));
                float inner = Smooth(0.45f, 0.86f, r) * (1 - Smooth(0.86f, 0.9f, r)) * 0.35f;
                float v = (edge + inner) * (0.55f + 0.6f * n);
                v *= 1 - Smooth(0.93f, 0.99f, r);
                int i = py * s + px;
                R[i] = 1; G[i] = 1; B[i] = 1; A[i] = C01(v);
            }
        Save(R, G, B, A, s, s, path);
    }

    // spark streak, vertical (V = along velocity for directional billboard)
    public static void Spark(string path, int w, int h)
    {
        float[] R = new float[w * h], G = new float[w * h], B = new float[w * h], A = new float[w * h];
        for (int py = 0; py < h; py++)
            for (int px = 0; px < w; px++)
            {
                float x = (px + 0.5f) / w * 2 - 1, y = (py + 0.5f) / h * 2 - 1;
                // head at the top (y=-1 in image space = top), tapering tail
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

    // debris chunks: cols x rows jagged polygons, dark metal with a hot orange rim
    public static void Debris(string path, int cols, int rows, int cell, int seed)
    {
        int W = cols * cell, H = rows * cell;
        float[] R = new float[W * H], G = new float[W * H], B = new float[W * H], A = new float[W * H];
        for (int v = 0; v < cols * rows; v++)
        {
            var rnd = new Random(seed + v * 13);
            int n = 4 + rnd.Next(4);
            float[] ra = new float[n];
            for (int k = 0; k < n; k++) ra[k] = 0.35f + (float)rnd.NextDouble() * 0.6f;
            float stretch = 0.6f + (float)rnd.NextDouble() * 0.4f;
            int ox = (v % cols) * cell, oy = (v / cols) * cell;
            for (int py = 0; py < cell; py++)
                for (int px = 0; px < cell; px++)
                {
                    float x = (px + 0.5f) / cell * 2 - 1, y = (py + 0.5f) / cell * 2 - 1;
                    y /= stretch;
                    float r = (float)Math.Sqrt(x * x + y * y);
                    float ang = (float)(Math.Atan2(y, x) + Math.PI) / (float)(Math.PI * 2) * n;
                    int k0 = (int)Math.Floor(ang) % n, k1 = (k0 + 1) % n;
                    float u = ang - (float)Math.Floor(ang);
                    float lim = Lerp(ra[k0], ra[k1], u);       // straight-ish facets
                    float fpx = 2f / cell;
                    float inside = C01((lim - r) / fpx + 0.5f);
                    float rim = Smooth(lim - 0.12f, lim - 0.01f, r) * (0.3f + 0.7f * C01(0.5f - y * 0.8f));
                    float facet = 0.5f + 0.5f * (float)Math.Sin(k0 * 2.1f + 1.3f);
                    float metal = 0.10f + 0.12f * facet;
                    float hot = rim * (0.6f + 0.4f * Noise(x * 6, y * 6, v, seed));
                    int i = (oy + py) * W + ox + px;
                    R[i] = metal + hot * 1.0f; G[i] = metal * 0.95f + hot * 0.42f; B[i] = metal * 0.9f + hot * 0.08f;
                    A[i] = inside;
                }
        }
        Save(R, G, B, A, W, H, path);
    }

    // preview: composite onto black with cell grid
    public static void Preview(string src, string dst, int cols, int rows)
    {
        using (var img = new Bitmap(src))
        using (var bmp = new Bitmap(img.Width, img.Height, PixelFormat.Format32bppArgb))
        using (var g = Graphics.FromImage(bmp))
        {
            g.Clear(Color.FromArgb(255, 18, 22, 34));
            g.DrawImage(img, 0, 0, img.Width, img.Height);
            using (var pen = new Pen(Color.FromArgb(255, 60, 200, 255)))
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

function P($name) { Join-Path $OutDir ("ED_" + $name + $Suffix + ".png") }

[ExTex]::FireSheet((P "Fire"), 4, 4, 256, 7)
[ExTex]::FireSheet((P "Fire2"), 4, 4, 256, 23)
[ExTex]::SmokeSheet((P "Smoke"), 2, 2, 256, 41)
[ExTex]::Glow((P "Glow"), 128, 2.2)
[ExTex]::Flash((P "Flash"), 256, 10, 5)
[ExTex]::Ring((P "Ring"), 256, 3)
[ExTex]::Spark((P "Spark"), 32, 128)
[ExTex]::Debris((P "Debris"), 2, 2, 128, 9)

[ExTex]::Preview((P "Fire"), (Join-Path $review "tex_fire.png"), 4, 4)
[ExTex]::Preview((P "Fire2"), (Join-Path $review "tex_fire2.png"), 4, 4)
[ExTex]::Preview((P "Smoke"), (Join-Path $review "tex_smoke.png"), 2, 2)
[ExTex]::Preview((P "Debris"), (Join-Path $review "tex_debris.png"), 2, 2)
[ExTex]::Preview((P "Flash"), (Join-Path $review "tex_flash.png"), 1, 1)
[ExTex]::Preview((P "Ring"), (Join-Path $review "tex_ring.png"), 1, 1)
"done"
