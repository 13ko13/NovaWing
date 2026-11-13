# 2面(雨の荒廃ビル街)用の曇り空スカイボックスを作る
# 使い方: powershell -ExecutionPolicy Bypass -File make_cloudy_sky.ps1
# 出力: ../skybox_front.png など6枚(1024x1024) と review/ にゲームのカメラ相当のプレビュー
# 面の向きは Game/BackGround/SkyBox.cpp の頂点の並びに合わせてある
# 雲は「方向」から計算するので、面のつなぎ目で模様が切れない

param(
    [int]$Size = 1024,
    [int]$Seed = 20261113
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$code = @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Threading.Tasks;

public static class CloudySky
{
    static int[] perm = new int[512];

    public static void Init(int seed)
    {
        var rnd = new Random(seed);
        int[] p = new int[256];
        for (int i = 0; i < 256; i++) p[i] = i;
        for (int i = 255; i > 0; i--) { int j = rnd.Next(i + 1); int t = p[i]; p[i] = p[j]; p[j] = t; }
        for (int i = 0; i < 512; i++) perm[i] = p[i & 255];
    }

    static double Fade(double t) { return t * t * t * (t * (t * 6 - 15) + 10); }
    static double Lerp(double a, double b, double t) { return a + (b - a) * t; }
    static double Grad(int h, double x, double y, double z)
    {
        h &= 15;
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

    // Improved Perlin noise, about -1..1
    public static double Noise(double x, double y, double z)
    {
        int X = (int)Math.Floor(x) & 255, Y = (int)Math.Floor(y) & 255, Z = (int)Math.Floor(z) & 255;
        x -= Math.Floor(x); y -= Math.Floor(y); z -= Math.Floor(z);
        double u = Fade(x), v = Fade(y), w = Fade(z);
        int A = perm[X] + Y, AA = perm[A] + Z, AB = perm[A + 1] + Z;
        int B = perm[X + 1] + Y, BA = perm[B] + Z, BB = perm[B + 1] + Z;
        return Lerp(
            Lerp(Lerp(Grad(perm[AA], x, y, z), Grad(perm[BA], x - 1, y, z), u),
                 Lerp(Grad(perm[AB], x, y - 1, z), Grad(perm[BB], x - 1, y - 1, z), u), v),
            Lerp(Lerp(Grad(perm[AA + 1], x, y, z - 1), Grad(perm[BA + 1], x - 1, y, z - 1), u),
                 Lerp(Grad(perm[AB + 1], x, y - 1, z - 1), Grad(perm[BB + 1], x - 1, y - 1, z - 1), u), v), w);
    }

    static double Fbm(double x, double y, double z, int oct)
    {
        double sum = 0, amp = 0.5, freq = 1, norm = 0;
        for (int i = 0; i < oct; i++)
        {
            sum += amp * Noise(x * freq, y * freq, z * freq);
            norm += amp;
            amp *= 0.5; freq *= 2.03;
        }
        return sum / norm;
    }

    static double Clamp01(double v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
    static double Smooth(double a, double b, double v) { double t = Clamp01((v - a) / (b - a)); return t * t * (3 - 2 * t); }

    // Colors (0-255)
    static readonly double[] Horizon = { 168, 174, 183 };   // 地平線のかすみ(フォグの色をこれに合わせる)
    static readonly double[] Zenith  = { 120, 128, 140 };   // 真上
    static readonly double[] CloudDark  = { 100, 107, 120 };  // 雲の厚い所
    static readonly double[] CloudLight = { 168, 174, 183 }; // 雲の薄い所
    static readonly double[] Scud = { 84, 91, 103 };         // 低くちぎれた暗い雲
    static readonly double[] Below = { 112, 118, 127 };      // 地平線より下(ビルと地面で大体隠れる)

    public static double[] SkyColor(double dx, double dy, double dz)
    {
        double len = Math.Sqrt(dx * dx + dy * dy + dz * dz);
        dx /= len; dy /= len; dz /= len;

        double[] c = new double[3];
        if (dy < 0)
        {
            double t = Smooth(0.0, 0.35, -dy);
            for (int i = 0; i < 3; i++) c[i] = Lerp(Horizon[i], Below[i], t);
            return c;
        }

        // 空のグラデーション(地平線が明るく、上ほど暗い)
        double g = Math.Pow(Smooth(0.0, 0.95, dy), 0.8);
        for (int i = 0; i < 3; i++) c[i] = Lerp(Horizon[i], Zenith[i], g);

        // 雲の層: 方向を高い所の平面へ投影(地平線近くほど遠くて細かく見える)
        double t1 = 1.0 / (dy + 0.10);
        double px = dx * t1 * 0.6, pz = dz * t1 * 0.6;
        double qx = Fbm(px + 3.1, pz - 1.7, 0.5, 3);
        double qz = Fbm(px - 4.6, pz + 2.9, 7.5, 3);
        double n = Fbm(px + 0.6 * qx, pz + 0.6 * qz, 2.0, 5);
        double cloud = Clamp01(0.5 + n * 1.1);
        cloud = Smooth(0.12, 0.88, cloud);
        double fade = Smooth(0.0, 0.30, dy);   // 地平線近くはかすみに溶かす(模様の細かすぎを防ぐ)
        double cover = 0.72 * fade;
        for (int i = 0; i < 3; i++) c[i] = Lerp(c[i], Lerp(CloudDark[i], CloudLight[i], cloud), cover);

        // 大きな雨雲の固まり(とても大きな模様で、ところどころ重く暗くする)
        double heavy = Smooth(-0.05, 0.35, Fbm(px * 0.35 + 20.0, pz * 0.35 - 8.0, 3.0, 3)) * fade * 0.45;
        for (int i = 0; i < 3; i++) c[i] = Lerp(c[i], CloudDark[i], heavy);

        // 低い層のちぎれ雲(暗い、少しだけ)
        double t2 = 1.0 / (dy + 0.05);
        double sx = dx * t2 * 1.2, sz = dz * t2 * 1.2;
        double s = Fbm(sx + 11.0 + 0.8 * qx, sz - 6.0, 13.0, 4);
        double scud = Smooth(0.05, 0.45, s) * Smooth(0.02, 0.22, dy) * (1.0 - Smooth(0.55, 0.9, dy)) * 0.4;
        for (int i = 0; i < 3; i++) c[i] = Lerp(c[i], Scud[i], scud);

        return c;
    }

    static void Store(byte[] buf, int idx, double[] c, Random r)
    {
        // ディザでグラデーションの段差を防ぐ
        double d = r.NextDouble() - 0.5;
        buf[idx + 0] = (byte)Math.Max(0, Math.Min(255, Math.Round(c[2] + d)));
        buf[idx + 1] = (byte)Math.Max(0, Math.Min(255, Math.Round(c[1] + d)));
        buf[idx + 2] = (byte)Math.Max(0, Math.Min(255, Math.Round(c[0] + d)));
    }

    static void Save(byte[] buf, int w, int h, string path)
    {
        using (var bmp = new Bitmap(w, h, PixelFormat.Format24bppRgb))
        {
            var data = bmp.LockBits(new Rectangle(0, 0, w, h), ImageLockMode.WriteOnly, PixelFormat.Format24bppRgb);
            for (int y = 0; y < h; y++)
                System.Runtime.InteropServices.Marshal.Copy(buf, y * w * 3, data.Scan0 + y * data.Stride, w * 3);
            bmp.UnlockBits(data);
            bmp.Save(path, ImageFormat.Png);
        }
    }

    // face: 0 front,1 back,2 left,3 right,4 up,5 down (SkyBox.cppの頂点と同じ向き)
    static void Dir(int face, double u, double v, out double x, out double y, out double z)
    {
        double a = -1 + 2 * u, b = 1 - 2 * v;
        switch (face)
        {
            case 0: x = a; y = b; z = 1; break;
            case 1: x = -a; y = b; z = -1; break;
            case 2: x = -1; y = b; z = a; break;
            case 3: x = 1; y = b; z = -a; break;
            case 4: x = a; y = 1; z = -1 + 2 * v; break;
            default: x = a; y = -1; z = 1 - 2 * v; break;
        }
    }

    public static void MakeFace(int face, int size, string path, int seed)
    {
        byte[] buf = new byte[size * size * 3];
        Parallel.For(0, size, py =>
        {
            var r = new Random(seed * 31 + face * 7919 + py);
            for (int px = 0; px < size; px++)
            {
                double x, y, z;
                Dir(face, (px + 0.5) / size, (py + 0.5) / size, out x, out y, out z);
                Store(buf, (py * size + px) * 3, SkyColor(x, y, z), r);
            }
        });
        Save(buf, size, size, path);
    }

    // 6枚の画像を方向から読む(SkyBox.cppの貼り方と同じになるようDirの逆算)
    static double[] SampleCube(byte[][] faces, int[] strides, int size, double x, double y, double z)
    {
        double ax = Math.Abs(x), ay = Math.Abs(y), az = Math.Abs(z);
        int face; double u, v;
        if (az >= ax && az >= ay)
        {
            if (z > 0) { face = 0; u = (x / z + 1) / 2; v = (1 - y / z) / 2; }
            else { face = 1; u = (x / z + 1) / 2; v = (1 - y / -z) / 2; }
        }
        else if (ax >= ay)
        {
            if (x < 0) { face = 2; u = (z / -x + 1) / 2; v = (1 - y / -x) / 2; }
            else { face = 3; u = (-z / x + 1) / 2; v = (1 - y / x) / 2; }
        }
        else
        {
            if (y > 0) { face = 4; u = (x / y + 1) / 2; v = (z / y + 1) / 2; }
            else { face = 5; u = (x / -y + 1) / 2; v = (1 - z / -y) / 2; }
        }
        int px = Math.Max(0, Math.Min(size - 1, (int)(u * size)));
        int py = Math.Max(0, Math.Min(size - 1, (int)(v * size)));
        int idx = py * strides[face] + px * 3;
        byte[] f = faces[face];
        return new double[] { f[idx + 2], f[idx + 1], f[idx] };
    }

    // 既存の6枚(paths: front,back,left,right,up,bottom)をゲームのカメラ相当で見る
    public static void MakeCubePreview(string[] paths, string outPath, double pitchDeg, double yawDeg)
    {
        byte[][] faces = new byte[6][];
        int[] strides = new int[6];
        int size = 0;
        for (int i = 0; i < 6; i++)
        {
            using (var src = new Bitmap(paths[i]))
            using (var bmp = src.Clone(new Rectangle(0, 0, src.Width, src.Height), PixelFormat.Format24bppRgb))
            {
                size = bmp.Width;
                var data = bmp.LockBits(new Rectangle(0, 0, size, size), ImageLockMode.ReadOnly, PixelFormat.Format24bppRgb);
                strides[i] = data.Stride;
                faces[i] = new byte[data.Stride * size];
                System.Runtime.InteropServices.Marshal.Copy(data.Scan0, faces[i], 0, faces[i].Length);
                bmp.UnlockBits(data);
            }
        }
        int w = 1280, h = 720;
        double aspect = (double)w / h;
        double p = pitchDeg * Math.PI / 180, yw = yawDeg * Math.PI / 180;
        byte[] buf = new byte[w * h * 3];
        Parallel.For(0, h, py =>
        {
            var r = new Random(py);
            for (int px = 0; px < w; px++)
            {
                double sx = ((px + 0.5) / w * 2 - 1) * aspect;
                double sy = 1 - (py + 0.5) / h * 2;
                double x = sx, y = sy, z = 1;
                double y2 = y * Math.Cos(p) + z * Math.Sin(p);
                double z2 = -y * Math.Sin(p) + z * Math.Cos(p);
                double x3 = x * Math.Cos(yw) + z2 * Math.Sin(yw);
                double z3 = -x * Math.Sin(yw) + z2 * Math.Cos(yw);
                Store(buf, (py * w + px) * 3, SampleCube(faces, strides, size, x3, y2, z3), r);
            }
        });
        Save(buf, w, h, outPath);
    }

    // ゲームのカメラ相当(縦の画角90度、1280x720)で、向きをpitch(上向き+)とyawで指定
    public static void MakePreview(string path, double pitchDeg, double yawDeg, int seed)
    {
        int w = 1280, h = 720;
        double tanHalf = Math.Tan(Math.PI / 4);
        double aspect = (double)w / h;
        double p = pitchDeg * Math.PI / 180, yw = yawDeg * Math.PI / 180;
        byte[] buf = new byte[w * h * 3];
        Parallel.For(0, h, py =>
        {
            var r = new Random(seed + py);
            for (int px = 0; px < w; px++)
            {
                double sx = ((px + 0.5) / w * 2 - 1) * tanHalf * aspect;
                double sy = (1 - (py + 0.5) / h * 2) * tanHalf;
                // カメラ空間(前+Z、右+X、上+Y)→pitch→yaw
                double x = sx, y = sy, z = 1;
                double y2 = y * Math.Cos(p) + z * Math.Sin(p);
                double z2 = -y * Math.Sin(p) + z * Math.Cos(p);
                double x3 = x * Math.Cos(yw) + z2 * Math.Sin(yw);
                double z3 = -x * Math.Sin(yw) + z2 * Math.Cos(yw);
                Store(buf, (py * w + px) * 3, SkyColor(x3, y2, z3), r);
            }
        });
        Save(buf, w, h, path);
    }
}
'@

Add-Type -TypeDefinition $code -ReferencedAssemblies System.Drawing

$outDir = Split-Path $PSScriptRoot -Parent
$reviewDir = Join-Path $PSScriptRoot 'review'
New-Item -ItemType Directory -Force $reviewDir | Out-Null

[CloudySky]::Init($Seed)
$names = 'front', 'back', 'left', 'right', 'up', 'bottom'
for ($f = 0; $f -lt 6; $f++) {
    $path = Join-Path $outDir ("skybox_{0}.png" -f $names[$f])
    [CloudySky]::MakeFace($f, $Size, $path, $Seed)
    Write-Host "wrote $path"
}

# プレビュー: 書き出した6枚を、SkyBox.cppと同じ貼り方で読んでゲームのカメラ相当で見る
# yaw45・pitch35は面のつなぎ目(前/右/上)をまたぐので、模様が切れていないかの確認になる
$cloudyPaths = $names | ForEach-Object { Join-Path $outDir ("skybox_{0}.png" -f $_) }
[CloudySky]::MakeCubePreview($cloudyPaths, (Join-Path $reviewDir 'gamecam_front.png'), 0, 0)
[CloudySky]::MakeCubePreview($cloudyPaths, (Join-Path $reviewDir 'gamecam_seam_yaw45_pitch35.png'), 35, 45)

# 貼り方の検算: 昼の空(ゲームで正しく表示されている)を同じ読み方で見て、つなぎ目が切れていなければ逆算が合っている
$dayDir = Join-Path (Split-Path $outDir -Parent) 'SkyBox'
$dayPaths = $names | ForEach-Object { Join-Path $dayDir ("skybox_{0}.png" -f $_) }
[CloudySky]::MakeCubePreview($dayPaths, (Join-Path $reviewDir 'check_day_seam_yaw45_pitch35.png'), 35, 45)
Write-Host 'preview done'
