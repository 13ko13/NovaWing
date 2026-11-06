# 弾の「暗い縁取り・裏打ち」用テクスチャ生成（2026-11-06）
# 明るい背景でも弾が見えるように、加算の芯の後ろに通常ブレンドの暗い層を敷くためのテクスチャ。
# 色はノードの色で付ける（ここではRGB=白〜灰、形はアルファ）。端は必ずアルファ0。
# usage: powershell -NoProfile -ExecutionPolicy Bypass -File gen_outline_textures.ps1
#   (リポジトリ直下で実行。EnemyBullet / PlayerBullet / PlayerChargeBullet の Texture に書き出す)

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @"
using System;
using System.Drawing;
using System.Drawing.Imaging;
public static class OutlineTex {
    static double C01(double v){ return v < 0 ? 0 : (v > 1 ? 1 : v); }
    static double Smooth(double a, double b, double x){ double t = C01((x - a) / (b - a)); return t * t * (3 - 2 * t); }

    // 円盤: 半径 r0 まで不透明、r1 で 0（はっきりした縁）。
    // RGB は中心 inGray → 縁 rimGray（rimA〜rimB で切り替え）。縁だけ暗くしたいときに使う。
    public static void Disc(string path, int size, double r0, double r1, int inGray, int rimGray, double rimA, double rimB){
        var bmp = new Bitmap(size, size, PixelFormat.Format32bppArgb);
        for (int y = 0; y < size; y++) for (int x = 0; x < size; x++){
            double u = ((x + 0.5) / size) * 2 - 1, v = ((y + 0.5) / size) * 2 - 1;
            double d = Math.Sqrt(u * u + v * v);
            double a = 1 - Smooth(r0, r1, d);
            double t = Smooth(rimA, rimB, d);
            int g = (int)Math.Round(inGray + (rimGray - inGray) * t);
            bmp.SetPixel(x, y, Color.FromArgb((int)Math.Round(a * 255), g, g, g));
        }
        bmp.Save(path, ImageFormat.Png);
    }

    // 軌跡(Track)用の帯: U(横)が断面、V(縦)は一様。|u| < w0 で不透明、w1 で 0。
    // RGB は中心 inGray → 縁 rimGray。alphaMax で全体の濃さ。
    public static void Strip(string path, int w, int h, double w0, double w1, double alphaMax, int inGray, int rimGray, double rimA, double rimB){
        var bmp = new Bitmap(w, h, PixelFormat.Format32bppArgb);
        for (int x = 0; x < w; x++){
            double u = Math.Abs(((x + 0.5) / w) * 2 - 1);
            double a = (1 - Smooth(w0, w1, u)) * alphaMax;
            double t = Smooth(rimA, rimB, u);
            int g = (int)Math.Round(inGray + (rimGray - inGray) * t);
            for (int y = 0; y < h; y++) bmp.SetPixel(x, y, Color.FromArgb((int)Math.Round(a * 255), g, g, g));
        }
        bmp.Save(path, ImageFormat.Png);
    }

    // 輪: 半径 rIn〜rOut が不透明（edge の幅で切り替え）。内側は inner の濃さで塗る（0 なら輪だけ）。RGB は白。
    public static void Ring(string path, int size, double rIn, double rOut, double edge, double inner){
        var bmp = new Bitmap(size, size, PixelFormat.Format32bppArgb);
        for (int y = 0; y < size; y++) for (int x = 0; x < size; x++){
            double u = ((x + 0.5) / size) * 2 - 1, v = ((y + 0.5) / size) * 2 - 1;
            double d = Math.Sqrt(u * u + v * v);
            double outer = 1 - Smooth(rOut - edge, rOut, d);
            double band = Smooth(rIn - edge, rIn, d) * outer;
            double a = Math.Max(band, inner * outer);
            bmp.SetPixel(x, y, Color.FromArgb((int)Math.Round(a * 255), 255, 255, 255));
        }
        bmp.Save(path, ImageFormat.Png);
    }

    // 針（火花）: 縦長。中央が太く、上下の端に向かって細くなる。縁ははっきり。RGB は白。
    public static void Needle(string path, int w, int h, double halfW, double edge){
        var bmp = new Bitmap(w, h, PixelFormat.Format32bppArgb);
        for (int y = 0; y < h; y++) for (int x = 0; x < w; x++){
            double u = Math.Abs(((x + 0.5) / w) * 2 - 1), v = Math.Abs(((y + 0.5) / h) * 2 - 1);
            double wv = halfW * (1 - Math.Pow(v, 1.6));
            double a = wv <= 0 ? 0 : 1 - Smooth(wv - edge, wv, u);
            if (v > 0.97) a = 0;
            bmp.SetPixel(x, y, Color.FromArgb((int)Math.Round(C01(a) * 255), 255, 255, 255));
        }
        bmp.Save(path, ImageFormat.Png);
    }
}
"@

$root = (Get-Location).Path
$eb = Join-Path $root "Data/Effect/EnemyBullet/Texture"
$pb = Join-Path $root "Data/Effect/PlayerBullet/Texture"
$cb = Join-Path $root "Data/Effect/PlayerChargeBullet/Texture"

# 敵の弾: 芯の後ろの円盤。中はノードの色（マゼンタ）のまま、縁だけ暗い（はっきりした縁取り）。
[OutlineTex]::Disc((Join-Path $eb "EB_Body2.png"), 128, 0.80, 0.90, 255, 60, 0.52, 0.70)
# 敵の弾: 尾。中はそのまま、縁だけ暗い（1ノードで縁取りになる）。
[OutlineTex]::Strip((Join-Path $eb "EB_TrailRim.png"), 64, 16, 0.72, 0.88, 0.95, 255, 70, 0.40, 0.68)

# プレイヤーの弾: 弾頭の後ろの暗い円盤と、軌跡の後ろの暗い帯。
[OutlineTex]::Disc((Join-Path $pb "PB_Backing.png"), 64, 0.70, 0.92, 255, 255, 0, 1)
[OutlineTex]::Strip((Join-Path $pb "PB_TrailBack.png"), 64, 16, 0.70, 0.90, 1.0, 255, 255, 0, 1)

# チャージ弾: 既存の Backing をはっきりした縁の円盤に、軌跡の後ろに暗い帯。
[OutlineTex]::Disc((Join-Path $cb "CB_Backing.png"), 128, 0.74, 0.90, 255, 255, 0, 1)
[OutlineTex]::Strip((Join-Path $cb "CB_TrailBack.png"), 64, 16, 0.70, 0.90, 1.0, 255, 255, 0, 1)

# (2026-11-06追加) 当たったときの火花: 暗い火花（はっきりした縁の針）と、暗い衝撃波の輪。
$he = Join-Path $root "Data/Effect/HitEffect/Texture"
[OutlineTex]::Needle((Join-Path $he "HE_SparkDark.png"), 32, 128, 0.80, 0.18)
[OutlineTex]::Ring((Join-Path $he "HE_RingDark.png"), 256, 0.80, 0.94, 0.03, 0)

# (2026-11-06追加) ボスのビーム: 当たり判定の太さの暗い紫の輪（太めの帯、中は薄く塗る）。遠くでも細くなりすぎないよう帯は太め。
$bb = Join-Path $root "Data/Effect/BossBeam/Texture"
[OutlineTex]::Ring((Join-Path $bb "BB_RimDisc2.png"), 256, 0.62, 0.94, 0.04, 0.35)
"done"
