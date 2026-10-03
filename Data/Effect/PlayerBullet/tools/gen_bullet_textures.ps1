# プレイヤー弾のテクスチャ生成（緑の光線）
# usage: powershell -NoProfile -ExecutionPolicy Bypass -File gen_bullet_textures.ps1 -OutDir <dir> [-Suffix v1]
param([string]$OutDir = ".", [string]$Suffix = "v1")

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @"
using System;
using System.Drawing;
using System.Drawing.Imaging;
public static class BulletTex {
    static double C01(double v){ return v < 0 ? 0 : (v > 1 ? 1 : v); }
    static double Smooth(double a, double b, double x){ double t = C01((x - a) / (b - a)); return t * t * (3 - 2 * t); }

    // 芯(白寄り) + グロー(緑) を合成して非乗算RGBAで書く
    static void Put(Bitmap bmp, int x, int y, double core, double glow, double[] coreCol, double[] glowCol){
        double a = C01(core + glow);
        double w = core + glow; if (w < 1e-6) w = 1e-6;
        double r = (coreCol[0] * core + glowCol[0] * glow) / w;
        double g = (coreCol[1] * core + glowCol[1] * glow) / w;
        double b = (coreCol[2] * core + glowCol[2] * glow) / w;
        bmp.SetPixel(x, y, Color.FromArgb((int)(a * 255), (int)r, (int)g, (int)b));
    }

    // 軌跡用: 横方向(U)に断面、縦方向(V)は一様
    public static void TrailProfile(string path, int w, int h, double coreW, double glowW, double glowA, double[] coreCol, double[] glowCol){
        var bmp = new Bitmap(w, h, PixelFormat.Format32bppArgb);
        for (int x = 0; x < w; x++){
            double u = ((x + 0.5) / w) * 2 - 1;
            double edge = 1 - Smooth(0.8, 1.0, Math.Abs(u));
            double core = Math.Exp(-Math.Pow(u / coreW, 2)) * edge;
            double glow = Math.Exp(-Math.Pow(u / glowW, 2)) * glowA * edge;
            for (int y = 0; y < h; y++) Put(bmp, x, y, core, glow, coreCol, glowCol);
        }
        bmp.Save(path, ImageFormat.Png);
    }

    // 弾頭用: 丸い光(芯 + 緑のにじみ)
    public static void HeadGlow(string path, int size, double coreR, double glowR, double glowA, double[] coreCol, double[] glowCol){
        var bmp = new Bitmap(size, size, PixelFormat.Format32bppArgb);
        for (int y = 0; y < size; y++) for (int x = 0; x < size; x++){
            double u = ((x + 0.5) / size) * 2 - 1, v = ((y + 0.5) / size) * 2 - 1;
            double d = Math.Sqrt(u * u + v * v);
            double edge = 1 - Smooth(0.8, 1.0, d);
            double core = Math.Exp(-Math.Pow(d / coreR, 2)) * edge;
            double glow = Math.Exp(-Math.Pow(d / glowR, 2)) * glowA * edge;
            Put(bmp, x, y, core, glow, coreCol, glowCol);
        }
        bmp.Save(path, ImageFormat.Png);
    }

    // 確認用: 黒背景に加算した見た目を横に並べる
    public static void Preview(string[] paths, string outPath, int cell){
        var bmp = new Bitmap(cell * paths.Length, cell, PixelFormat.Format32bppArgb);
        using (var g = Graphics.FromImage(bmp)){
            g.Clear(Color.Black);
            for (int i = 0; i < paths.Length; i++){
                using (var src = new Bitmap(paths[i])){
                    var tmp = new Bitmap(src.Width, src.Height, PixelFormat.Format32bppArgb);
                    for (int y = 0; y < src.Height; y++) for (int x = 0; x < src.Width; x++){
                        var c = src.GetPixel(x, y); double a = c.A / 255.0;
                        tmp.SetPixel(x, y, Color.FromArgb(255, (int)(c.R * a), (int)(c.G * a), (int)(c.B * a)));
                    }
                    g.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.NearestNeighbor;
                    g.DrawImage(tmp, i * cell, 0, cell, cell);
                    g.DrawRectangle(Pens.DimGray, i * cell, 0, cell - 1, cell - 1);
                }
            }
        }
        bmp.Save(outPath, ImageFormat.Png);
    }
}
"@

New-Item -ItemType Directory -Force $OutDir | Out-Null
$OutDir = (Resolve-Path $OutDir).Path
$core = [double[]](235, 255, 238)   # 芯: 白寄りの黄緑
$glow = [double[]](30, 255, 110)    # にじみ: エメラルド寄りの緑

$trail = Join-Path $OutDir "BulletTrail_$Suffix.png"
$head  = Join-Path $OutDir "BulletHead_$Suffix.png"
[BulletTex]::TrailProfile($trail, 64, 16, 0.17, 0.5, 0.6, $core, $glow)
[BulletTex]::HeadGlow($head, 64, 0.2, 0.55, 0.7, $core, $glow)
[BulletTex]::Preview(@($trail, $head), (Join-Path $OutDir "preview_$Suffix.png"), 256)
"done: $trail, $head"
