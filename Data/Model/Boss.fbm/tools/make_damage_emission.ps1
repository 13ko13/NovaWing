# ボスの顔を赤く光らせる用の発光マップ(T_Mech_LOD2_E_Damage.png)を作る。
# 元の発光マップ(T_Mech_LOD2_E.png、4096x4096)の顔の範囲だけ、黄色を明るい赤に置き換える。
# 顔以外(目の丸・線など)は元の色のまま。出力は2048x2048(メモリ節約。UVで読むので大きさが違っても位置はずれない)。
# 使い方: powershell -NoProfile -ExecutionPolicy Bypass -File make_damage_emission.ps1 [-Size 4096]
# ※日本語を含むのでBOM付きUTF-8で保存すること(PowerShell 5.1で文字化けしないように)
param([int]$Size = 2048)

$fbm = Split-Path -Parent $PSScriptRoot   # ...\Data\Model\Boss.fbm
$src = Join-Path $fbm "T_Mech_LOD2_E.png"
$dst = Join-Path $fbm "T_Mech_LOD2_E_Damage.png"

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @"
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class BossDamageEmission
{
    // 顔の範囲(元の4096x4096での画素座標)。右隣の黄色い丸(x=1635〜)は含めない
    const int FaceLeft = 950, FaceTop = 2740, FaceRight = 1620, FaceBottom = 3260;
    // 赤の色(明るさ1のとき)。G/Bを少し残して、真っ赤より明るく光って見えるようにする
    const float R = 255f, G = 25f, B = 20f;

    public static void Make(string srcPath, string dstPath, int size)
    {
        using (Bitmap src = new Bitmap(srcPath))
        using (Bitmap work = new Bitmap(src.Width, src.Height, PixelFormat.Format32bppArgb))
        {
            using (Graphics g = Graphics.FromImage(work))
            {
                g.DrawImage(src, new Rectangle(0, 0, src.Width, src.Height));
            }
            Rectangle all = new Rectangle(0, 0, work.Width, work.Height);
            BitmapData data = work.LockBits(all, ImageLockMode.ReadWrite, PixelFormat.Format32bppArgb);
            int stride = data.Stride;
            byte[] px = new byte[stride * work.Height];
            Marshal.Copy(data.Scan0, px, 0, px.Length);

            // 元画像が4096以外でも同じ場所を指すように、範囲を割合で換算する
            float sx = work.Width / 4096f, sy = work.Height / 4096f;
            int x0 = (int)(FaceLeft * sx), x1 = (int)(FaceRight * sx);
            int y0 = (int)(FaceTop * sy), y1 = (int)(FaceBottom * sy);
            for (int y = y0; y < y1; y++)
            {
                for (int x = x0; x < x1; x++)
                {
                    int i = y * stride + x * 4;   // BGRA
                    byte b = px[i], gg = px[i + 1], r = px[i + 2];
                    // 明るさ = 一番強いチャンネル(汚れの濃淡をそのまま残す)
                    float lum = Math.Max(r, Math.Max(gg, b)) / 255f;
                    px[i + 2] = (byte)Math.Round(R * lum);
                    px[i + 1] = (byte)Math.Round(G * lum);
                    px[i]     = (byte)Math.Round(B * lum);
                }
            }
            Marshal.Copy(px, 0, data.Scan0, px.Length);
            work.UnlockBits(data);

            using (Bitmap outBmp = new Bitmap(size, size, PixelFormat.Format32bppArgb))
            {
                using (Graphics g = Graphics.FromImage(outBmp))
                {
                    g.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.HighQualityBicubic;
                    g.PixelOffsetMode = System.Drawing.Drawing2D.PixelOffsetMode.HighQuality;
                    g.CompositingMode = System.Drawing.Drawing2D.CompositingMode.SourceCopy;
                    using (ImageAttributes attr = new ImageAttributes())
                    {
                        // 縁で外側の色を拾わないようにする
                        attr.SetWrapMode(System.Drawing.Drawing2D.WrapMode.TileFlipXY);
                        g.DrawImage(work, new Rectangle(0, 0, size, size), 0, 0, work.Width, work.Height, GraphicsUnit.Pixel, attr);
                    }
                }
                outBmp.Save(dstPath, ImageFormat.Png);
            }
        }
    }
}
"@

[BossDamageEmission]::Make($src, $dst, $Size)
"wrote $dst"
