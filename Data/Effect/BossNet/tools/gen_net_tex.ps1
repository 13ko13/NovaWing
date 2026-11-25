# 網のレーザー用テクスチャを作る。実行: powershell -NoProfile -ExecutionPolicy Bypass -File gen_net_tex.ps1
# 出力先: ../Texture/
# v6(11/25 軽量化): 線は「Y軸固定ビルボード」(板の縦=線の長手方向、カメラの方へ線を軸に回る)で描くので、
#   線のテクスチャは 横(U)=線の太さ方向の断面、縦(V)=線の長手方向(どこも同じ)。端(U=0,1)で alpha=0。
#   重なっていた層は1枚に焼き込んだ(NB_LineBake / NB_Arc3 / NB_StreakBake)。
#   焼き込みの色は「エフェクトの層を重ねた結果」を計算して入れている(下の Bake* の説明)。
Add-Type -AssemblyName System.Drawing

$outDir = Join-Path $PSScriptRoot '..\Texture'
New-Item -ItemType Directory -Force $outDir | Out-Null
$outDir = (Resolve-Path $outDir).Path

function Save-Tex([int]$w, [int]$h, [scriptblock]$alphaFn, [string]$name) {
    $bmp = New-Object System.Drawing.Bitmap($w, $h, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    for ($y = 0; $y -lt $h; $y++) {
        for ($x = 0; $x -lt $w; $x++) {
            $u = ($x + 0.5) / $w
            $v = ($y + 0.5) / $h
            $a = [double](& $alphaFn $u $v)
            if ($a -lt 0) { $a = 0 } elseif ($a -gt 1) { $a = 1 }
            $bmp.SetPixel($x, $y, [System.Drawing.Color]::FromArgb([int][math]::Round($a * 255), 255, 255, 255))
        }
    }
    $bmp.Save((Join-Path $outDir $name), [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Host "saved $name"
}

function Smooth([double]$e0, [double]$e1, [double]$x) {
    $t = ($x - $e0) / ($e1 - $e0)
    if ($t -lt 0) { $t = 0 } elseif ($t -gt 1) { $t = 1 }
    return $t * $t * (3 - 2 * $t)
}

# 縁(t=1)で必ず0になる断面。t = 中心からの距離(0〜1)。横(U)=線の長手方向、縦(V)=断面(予告の線用。予告は今まで通り固定の板)
# Flat: ほぼ一様で縁だけ柔らかい(予告の暗い層用)
Save-Tex 16 128 { param($u, $v) $t = [math]::Abs($v - 0.5) * 2; 1 - (Smooth 0.72 1.0 $t) } 'NB_Flat.png'
# Core: 細く鋭い芯(予告の点滅する芯用)
Save-Tex 16 128 { param($u, $v) $t = [math]::Abs($v - 0.5) * 2; [math]::Exp(-5.0 * $t * $t) * (1 - (Smooth 0.6 1.0 $t)) } 'NB_Core.png'

# Head: 先端の光(丸い芯 + 十字の光条)。端で0
Save-Tex 128 128 {
    param($u, $v)
    $x = ($u - 0.5) * 2; $y = ($v - 0.5) * 2
    $r = [math]::Sqrt($x * $x + $y * $y)
    $edge = 1 - (Smooth 0.7 1.0 $r)
    $glow = [math]::Exp(-9 * $r * $r)
    $ax = [math]::Abs($x); $ay = [math]::Abs($y)
    $star = 0.55 * ([math]::Exp(-$ax * 3) * [math]::Exp(-$ay * 28) + [math]::Exp(-$ay * 3) * [math]::Exp(-$ax * 28))
    ($glow + $star) * $edge
} 'NB_Head.png'

# Arc: 線に沿って走るビリビリ(ギザギザの電気)。v2(11/25「全然ビリビリしてない、がっつり」): 振れ幅・芯の太さを上げ、途切れを減らし、筋を3本に。
#   線の長さ140(エディタ単位)×太さ4.0に引き伸ばされる前提で、距離は見た目の縦横比で測る。
#   v6: 以前の2層(Arc と、太さ0.8倍・上下反転・2Fごとの Arc2)を1枚に焼き込んだ NB_Arc3 にした。
#   コマ f = Arc のコマ f + Arc2 として別のコマ(f+3)を上下反転・0.8倍で重ねたもの。横に8コマ(1コマ 64x1024)。
# Spark: 線から飛び出す短い稲妻(枝分かれあり)。4x2コマ、1コマ128。線上のランダムな位置に回転を変えて出す。
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
public static class NetArc {
    static double Smooth(double e0, double e1, double x) { double t = (x - e0) / (e1 - e0); if (t < 0) t = 0; if (t > 1) t = 1; return t * t * (3 - 2 * t); }
    // 中点変位法で折れ線(y は見た目の単位、太さ方向)を作る。
    // 変位 = 区間の長さ × kink(上限 cap)。細かい区間でも変位を減らしすぎないのでギザギザが残る
    static double[] Bolt(Random r, int depth, double lenUnits, double kink, double cap, double limit) {
        int n = 1 << depth; double[] y = new double[n + 1];
        y[0] = 0; y[n] = 0;
        for (int step = n; step > 1; step /= 2) {
            double seg = lenUnits * step / n;
            double amp = Math.Min(cap, seg * kink);
            for (int i = 0; i < n; i += step) {
                int m = i + step / 2;
                y[m] = (y[i] + y[i + step]) * 0.5 + (r.NextDouble() * 2 - 1) * amp;
                y[m] = Math.Max(-limit, Math.Min(limit, y[m]));
            }
        }
        return y;
    }
    // 電気のコマ(alpha、[長手方向px, 太さ方向px])を作る。乱数の順番は v2 の NB_Arc2 と同じなので同じ稲妻になる
    public static double[][,] ArcFrames(int frames, int fw, int fh, double lenUnits, double widthUnits, int seed) {
        var res = new double[frames][,];
        var r = new Random(seed);
        double sx = lenUnits / fw, sy = widthUnits / fh;   // 1px の見た目の大きさ
        for (int f = 0; f < frames; f++) {
            var arr = new double[fw, fh];
            int depth = 7; int n = 1 << depth;
            double[] y = Bolt(r, depth, lenUnits, 0.8, 0.9, widthUnits * 0.38);
            double[] y2 = Bolt(r, depth, lenUnits, 0.8, 0.7, widthUnits * 0.34);
            double[] y3 = Bolt(r, depth, lenUnits, 0.8, 0.6, widthUnits * 0.30);
            // 途切れ: 区間ごとに点いている/消えている。約8割が点く
            int segs = 24; double[] on = new double[segs];
            for (int i = 0; i < segs; i++) on[i] = r.NextDouble() < 0.8 ? 0.7 + 0.3 * r.NextDouble() : 0.15;
            for (int py = 0; py < fh; py++) {
                double v = (py + 0.5) / fh;
                double edge = 1 - Smooth(0.38, 0.5, Math.Abs(v - 0.5));
                for (int px = 0; px < fw; px++) {
                    double u = (px + 0.5) / fw;
                    // 途切れの強さ(区間の境はなめらかに)
                    double sp = u * segs - 0.5; int s0 = (int)Math.Floor(sp); double st = sp - s0;
                    double a0 = on[((s0 % segs) + segs) % segs], a1 = on[(((s0 + 1) % segs) + segs) % segs];
                    double gate = a0 + (a1 - a0) * Smooth(0, 1, st);
                    double best = 1e9, best2 = 1e9, best3 = 1e9;
                    double fx = u * n; int k0 = Math.Max(0, (int)fx - 2), k1 = Math.Min(n - 1, (int)fx + 2);
                    for (int k = k0; k <= k1; k++) {
                        best = Math.Min(best, SegDist(px, py, k, y, n, fw, fh, sx, sy));
                        best2 = Math.Min(best2, SegDist(px, py, k, y2, n, fw, fh, sx, sy));
                        best3 = Math.Min(best3, SegDist(px, py, k, y3, n, fw, fh, sx, sy));
                    }
                    double core = Math.Exp(-Math.Pow(best / 0.09, 2));
                    double glow = 0.5 * Math.Exp(-best / 0.25);
                    double sub = 0.8 * Math.Exp(-Math.Pow(best2 / 0.06, 2)) + 0.3 * Math.Exp(-best2 / 0.18);
                    double sub3 = 0.6 * Math.Exp(-Math.Pow(best3 / 0.05, 2)) + 0.2 * Math.Exp(-best3 / 0.15);
                    arr[px, py] = Math.Max(0, Math.Min(1, Math.Min(1, core + glow + sub + sub3) * gate * edge));
                }
            }
            res[f] = arr;
        }
        return res;
    }
    // 点(px,py)と k 番目の線分の、見た目の単位での距離
    static double SegDist(int px, int py, int k, double[] y, int n, int fw, int fh, double sx, double sy) {
        double mid = fh * sy * 0.5;
        double ax = (double)k / n * fw * sx, ay = mid + y[k];
        double bx = (double)(k + 1) / n * fw * sx, by = mid + y[k + 1];
        double qx = (px + 0.5) * sx, qy = (py + 0.5) * sy;
        double dx = bx - ax, dy = by - ay; double t = ((qx - ax) * dx + (qy - ay) * dy) / (dx * dx + dy * dy);
        if (t < 0) t = 0; if (t > 1) t = 1;
        double ex = ax + dx * t - qx, ey = ay + dy * t - qy;
        return Math.Sqrt(ex * ex + ey * ey);
    }
    // a[長手, 太さ] を 0〜1 の座標で双線形に取り出す(外は0)
    static double Sample(double[,] a, double along, double across) {
        int w = a.GetLength(0), h = a.GetLength(1);
        double x = along * w - 0.5, y = across * h - 0.5;
        int x0 = (int)Math.Floor(x), y0 = (int)Math.Floor(y); double tx = x - x0, ty = y - y0;
        double v00 = At(a, x0, y0), v10 = At(a, x0 + 1, y0), v01 = At(a, x0, y0 + 1), v11 = At(a, x0 + 1, y0 + 1);
        return (v00 * (1 - tx) + v10 * tx) * (1 - ty) + (v01 * (1 - tx) + v11 * tx) * ty;
    }
    static double At(double[,] a, int i, int j) {
        int w = a.GetLength(0), h = a.GetLength(1);
        if (j < 0 || j >= h) return 0;
        if (i < 0) i = 0; if (i >= w) i = w - 1;
        return a[i, j];
    }
    // 加算用の画素を書く。k = 足したい光(色×アルファ、0〜1)。alpha = 一番明るいチャンネル、色 = k / alpha
    static void PutAdd(Bitmap bmp, int x, int y, double kr, double kg, double kb) {
        double a = Math.Min(1, Math.Max(kr, Math.Max(kg, kb)));
        if (a < 1e-5) { bmp.SetPixel(x, y, Color.FromArgb(0, 255, 255, 255)); return; }
        bmp.SetPixel(x, y, Color.FromArgb(B(a), B(kr / a), B(kg / a), B(kb / a)));
    }
    static int B(double v) { return (int)Math.Round(Math.Max(0, Math.Min(1, v)) * 255); }

    // NB_Arc3: 横に frames コマ(1コマ outAcross x outAlong、横=太さ方向、縦=長手方向)。
    // kA/kB = Arc/Arc2 の色×アルファ(以前の乱数の最大値)。Arc2 は太さ bScale 倍・上下反転・コマを bShift ずらす
    public static void SaveArcMerged(string path, double[][,] fr, int outAcross, int outAlong, double[] kA, double[] kB, double bScale, int bShift) {
        int n = fr.Length;
        var bmp = new Bitmap(outAcross * n, outAlong, PixelFormat.Format32bppArgb);
        for (int f = 0; f < n; f++)
            for (int py = 0; py < outAlong; py++)
                for (int px = 0; px < outAcross; px++) {
                    double along = (py + 0.5) / outAlong, across = (px + 0.5) / outAcross;
                    double tA = Sample(fr[f], along, across);
                    double ub = 1 - (0.5 + (across - 0.5) / bScale);
                    double tB = (ub < 0 || ub > 1) ? 0 : Sample(fr[(f + bShift) % n], along, ub);
                    PutAdd(bmp, f * outAcross + px, py, kA[0] * tA + kB[0] * tB, kA[1] * tA + kB[1] * tB, kA[2] * tA + kB[2] * tB);
                }
        bmp.Save(path, ImageFormat.Png); bmp.Dispose();
    }

    // ---- 線の断面(t = 中心からの距離 / その層の半分の太さ) ----
    static double Flat(double t) { return t >= 1 ? 0 : 1 - Smooth(0.72, 1.0, t); }
    static double Soft(double t) { return t >= 1 ? 0 : Math.Pow(1 - t, 1.6); }
    static double CoreP(double t) { return t >= 1 ? 0 : Math.Exp(-5.0 * t * t) * (1 - Smooth(0.6, 1.0, t)); }
    static double[] Lerp(double[] a, double[] b, double t) { var r = new double[a.Length]; for (int i = 0; i < a.Length; i++) r[i] = a[i] + (b[i] - a[i]) * t; return r; }

    // NB_LineBake: 本番の線の4層(暗い縁 Blend + 本体 Blend + 光 Add + 芯 Add)を1枚の Blend 用に焼く。
    // 背景 bg に重ねた結果は (1-α)bg + K の形になるので、alpha=α、色=K/α にすれば Blend 1枚で同じになる
    // 注意: 寿命1Fを毎フレーム出し直す層は、消える前の1個と新しい1個が重なって「2回」描かれる(エディタで確認)。
    //   以前の4層もこの焼いた板も2回描かれるので、目標は「各層を2回ずつ重ねた結果」:
    //   aD2 = 1-(1-aD)^2、aB2 = 1-(1-aB)^2、αt = 1-(1-aD2)(1-aB2)、K = (1-aB2)·aD2·D + aB2·B + 2G + 2C
    //   板を2回重ねると alpha は 1-(1-α)^2 になるので、板の alpha は α = 1-(1-aD)(1-aB)、色は K/αt
    // 光(G)は以前、出し直すたびに明るさが乱数で揺れていたので、明るさを変えたコマを横に frames 枚並べ、コマを乱数で選ぶ。
    // halfW = 板の半分の太さ。各層の半分の太さ hD/hB/hG/hC。色はすべて 0〜1、gMin/gMax は RGBA(光の乱数の範囲)
    public static void SaveLineBake(string path, int across, int along, int frames, double halfW,
                                    double[] D, double aDk, double hD, double[] Bc, double hB,
                                    double[] gMin, double[] gMax, double hG, double[] Cc, double hC) {
        var bmp = new Bitmap(across * frames, along, PixelFormat.Format32bppArgb);
        for (int f = 0; f < frames; f++) {
            var g = Lerp(gMin, gMax, frames == 1 ? 1 : (double)f / (frames - 1));
            for (int px = 0; px < across; px++) {
                double y = Math.Abs(((px + 0.5) / across - 0.5) * 2 * halfW);
                double aD = aDk * Flat(y / hD), aB = Flat(y / hB);
                double gs = g[3] * Soft(y / hG), cs = Cc[3] * CoreP(y / hC);
                double al = 1 - (1 - aD) * (1 - aB);
                double aD2 = 1 - (1 - aD) * (1 - aD), aB2 = 1 - (1 - aB) * (1 - aB);
                double alt = 1 - (1 - aD2) * (1 - aB2);
                double[] K = new double[3];
                for (int c = 0; c < 3; c++) K[c] = (1 - aB2) * aD2 * D[c] + aB2 * Bc[c] + 2 * (g[c] * gs + Cc[c] * cs);
                Color col;
                if (al < 1e-5) col = Color.FromArgb(0, 0, 0, 0);
                else col = Color.FromArgb(B(al), B(K[0] / alt), B(K[1] / alt), B(K[2] / alt));
                for (int py = 0; py < along; py++) bmp.SetPixel(f * across + px, py, col);
            }
        }
        bmp.Save(path, ImageFormat.Png); bmp.Dispose();
    }

    // NB_StreakBake: 光の筋の2層(光 Add + 芯 Add)を1枚の Add 用に焼く。光の明るさ違いのコマを横に frames 枚
    public static void SaveStreakBake(string path, int across, int along, int frames, double halfW,
                                      double[] gMin, double[] gMax, double hG, double[] Cc, double hC) {
        var bmp = new Bitmap(across * frames, along, PixelFormat.Format32bppArgb);
        for (int f = 0; f < frames; f++) {
            var g = Lerp(gMin, gMax, frames == 1 ? 1 : (double)f / (frames - 1));
            for (int px = 0; px < across; px++) {
                double y = Math.Abs(((px + 0.5) / across - 0.5) * 2 * halfW);
                double gs = g[3] * Soft(y / hG), cs = Cc[3] * CoreP(y / hC);
                for (int py = 0; py < along; py++)
                    PutAdd(bmp, f * across + px, py, g[0] * gs + Cc[0] * cs, g[1] * gs + Cc[1] * cs, g[2] * gs + Cc[2] * cs);
            }
        }
        bmp.Save(path, ImageFormat.Png); bmp.Dispose();
    }

    // ---- Spark ----
    // 2Dの中点変位で稲妻を作り、線分を list に足す。branch>0 なら途中から枝を出す
    static void Bolt2D(System.Collections.Generic.List<double[]> segs, Random r, double ax, double ay, double bx, double by, double disp, int depth, int branch) {
        if (depth == 0) { segs.Add(new double[] { ax, ay, bx, by }); return; }
        double mx = (ax + bx) * 0.5, my = (ay + by) * 0.5;
        double dx = bx - ax, dy = by - ay; double len = Math.Sqrt(dx * dx + dy * dy);
        double off = (r.NextDouble() * 2 - 1) * disp * len;
        mx += -dy / len * off; my += dx / len * off;
        Bolt2D(segs, r, ax, ay, mx, my, disp, depth - 1, branch);
        Bolt2D(segs, r, mx, my, bx, by, disp, depth - 1, branch);
        if (branch > 0 && depth >= 3 && r.NextDouble() < 0.45) {
            double ang = Math.Atan2(dy, dx) + (r.NextDouble() < 0.5 ? -1 : 1) * (0.4 + 0.5 * r.NextDouble());
            double bl = len * (0.35 + 0.3 * r.NextDouble());
            Bolt2D(segs, r, mx, my, mx + Math.Cos(ang) * bl, my + Math.Sin(ang) * bl, disp, depth - 2, branch - 1);
        }
    }
    public static void SaveSpark(string path, int cell, int cols, int rows, int seed) {
        var bmp = new Bitmap(cell * cols, cell * rows, PixelFormat.Format32bppArgb);
        var r = new Random(seed);
        for (int f = 0; f < cols * rows; f++) {
            var segs = new System.Collections.Generic.List<double[]>();
            Bolt2D(segs, r, -0.85, (r.NextDouble() - 0.5) * 0.4, 0.85, (r.NextDouble() - 0.5) * 0.4, 0.28, 6, 2);
            int ox = (f % cols) * cell, oy = (f / cols) * cell;
            for (int py = 0; py < cell; py++) for (int px = 0; px < cell; px++) {
                double x = (px + 0.5) / cell * 2 - 1, y = (py + 0.5) / cell * 2 - 1;
                double d = 10;
                foreach (var s in segs) {
                    double vx = s[2] - s[0], vy = s[3] - s[1];
                    double t = ((x - s[0]) * vx + (y - s[1]) * vy) / (vx * vx + vy * vy + 1e-12);
                    if (t < 0) t = 0; if (t > 1) t = 1;
                    double ex = s[0] + vx * t - x, ey = s[1] + vy * t - y;
                    d = Math.Min(d, Math.Sqrt(ex * ex + ey * ey));
                }
                double core = Math.Exp(-Math.Pow(d / 0.03, 2));
                double glow = 0.6 * Math.Exp(-d / 0.09);
                double edgePx = Math.Min(Math.Min(px, cell - 1 - px), Math.Min(py, cell - 1 - py));
                double edge = Math.Min(1, edgePx / 10.0);
                double a = Math.Min(1, core + glow) * edge;
                bmp.SetPixel(ox + px, oy + py, Color.FromArgb((int)Math.Round(a * 255), 255, 255, 255));
            }
        }
        bmp.Save(path, ImageFormat.Png); bmp.Dispose();
    }
}
'@
# 色(0〜255)を 0〜1 に
function C01([double[]]$c) { [double[]]($c | ForEach-Object { $_ / 255.0 }) }
# 加算の層の「色×アルファ」(0〜1)
function K01([double[]]$c) { $a = $c[3] / 255.0; [double[]]@((($c[0] / 255.0) * $a), (($c[1] / 255.0) * $a), (($c[2] / 255.0) * $a)) }

# ---- 本番の線(NB_LineBake): 以前の Dark/Body/Glow/Core の数値(make_efkproj.ps1 v4)を焼く ----
# 板の太さ1.4(= Dark/Glow の太さ)。Dark 1.4、Body 1.0、Glow 1.4、Core 0.4。光は8段階
[NetArc]::SaveLineBake((Join-Path $outDir 'NB_LineBake.png'), 128, 8, 8, 0.7,
    (C01 @(12, 58, 30)), (170 / 255.0), 0.7,
    (C01 @(60, 190, 95)), 0.5,
    (C01 @(80, 255, 130, 45)), (C01 @(140, 255, 180, 90)), 0.7,
    (C01 @(225, 255, 230, 155)), 0.2)
Write-Host "saved NB_LineBake.png"

# ---- 電気(NB_Arc3): Arc(色の最大 150,255,180,120)+ Arc2(最大 100,240,140,80、太さ0.8倍・上下反転) ----
# Arc3 は寿命1Fで毎フレーム出し直す(=いつも2回描かれる)。以前の Arc2 は寿命2Fを2Fごと(1回と2回が交互=平均1.5回)だったので、Arc2 の分は 0.75 倍で焼く
$arcFrames = [NetArc]::ArcFrames(8, 1024, 64, 140.0, 4.0, 7)
$kArc2 = K01 @(100, 240, 140, 80); $kArc2 = [double[]]($kArc2 | ForEach-Object { $_ * 0.75 })
[NetArc]::SaveArcMerged((Join-Path $outDir 'NB_Arc3.png'), $arcFrames, 64, 1024,
    (K01 @(150, 255, 180, 120)), $kArc2, 0.8, 3)
Write-Host "saved NB_Arc3.png"

# ---- 光の筋(NB_StreakBake): 以前の Glow(太さ0.7、乱数 70,255,120,70〜120,255,165,120)+ Core(太さ0.18、215,255,225,210) ----
[NetArc]::SaveStreakBake((Join-Path $outDir 'NB_StreakBake.png'), 128, 8, 8, 0.35,
    (C01 @(70, 255, 120, 70)), (C01 @(120, 255, 165, 120)), 0.35,
    (C01 @(215, 255, 225, 210)), 0.09)
Write-Host "saved NB_StreakBake.png"

[NetArc]::SaveSpark((Join-Path $outDir 'NB_Spark2.png'), 128, 4, 2, 11)
Write-Host "saved NB_Spark2.png"
