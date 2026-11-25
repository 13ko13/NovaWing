# 網のレーザー用テクスチャを作る。実行: powershell -NoProfile -ExecutionPolicy Bypass -File gen_net_tex.ps1
# 出力先: ../Texture/
# 線のテクスチャは、横(U)=線の長手方向で同じ、縦(V)=線の太さ方向の断面。端(V=0,1)で alpha=0。
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

# 縁(t=1)で必ず0になる断面。t = 中心からの距離(0〜1)
# Flat: ほぼ一様で縁だけ柔らかい(暗い層・本体の層用)
Save-Tex 16 128 { param($u, $v) $t = [math]::Abs($v - 0.5) * 2; 1 - (Smooth 0.72 1.0 $t) } 'NB_Flat.png'
# Soft: なだらかな光(外側のにじみ用)
Save-Tex 16 128 { param($u, $v) $t = [math]::Abs($v - 0.5) * 2; $f = 1 - $t; [math]::Pow($f, 1.6) } 'NB_Soft.png'
# Core: 細く鋭い芯
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

# Arc: 線に沿って走るビリビリ(ギザギザの電気)。縦に8コマ並べたフリップブック(1コマ 1024x64)。
# エフェクト側でコマを乱数で選び、出し直すたびにチラつかせる。
# 線の長さ140(エディタ単位)×太さ4.0に引き伸ばされる前提で、距離は見た目の縦横比で測る。
# v2(11/25「全然ビリビリしてない、がっつり」): 振れ幅・芯の太さを上げ、途切れを減らし、筋を3本に。
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
    public static void Save(string path, int frames, int fw, int fh, double lenUnits, double widthUnits, int seed) {
        var bmp = new Bitmap(fw, fh * frames, PixelFormat.Format32bppArgb);
        var r = new Random(seed);
        double sx = lenUnits / fw, sy = widthUnits / fh;   // 1px の見た目の大きさ
        for (int f = 0; f < frames; f++) {
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
                    double a = Math.Min(1, core + glow + sub + sub3) * gate * edge;
                    int ai = (int)Math.Round(Math.Max(0, Math.Min(1, a)) * 255);
                    bmp.SetPixel(px, f * fh + py, Color.FromArgb(ai, 255, 255, 255));
                }
            }
        }
        bmp.Save(path, ImageFormat.Png); bmp.Dispose();
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
# 名前を変えたのはエディタのテクスチャキャッシュを避けるため(NB_Arc.png は v1)
[NetArc]::Save((Join-Path $outDir 'NB_Arc2.png'), 8, 1024, 64, 140.0, 4.0, 7)
Write-Host "saved NB_Arc2.png"
[NetArc]::SaveSpark((Join-Path $outDir 'NB_Spark2.png'), 128, 4, 2, 11)
Write-Host "saved NB_Spark2.png"
