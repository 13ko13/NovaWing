# 2面の雨の素材を作る（2026-11-11）
#   Texture/Rain_Streak.png : 雨の筋（白、縦長。両端と左右の縁で alpha=0。色はノードで付ける）
# 使い方: powershell -NoProfile -ExecutionPolicy Bypass -File gen_rain.ps1 -OutDir <Rainフォルダ>
param([string]$OutDir = ".", [string]$Name = "Rain_Streak.png")
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @"
using System; using System.Drawing; using System.Drawing.Imaging;
public static class RainTex {
  static double C01(double x){ return x<0?0:(x>1?1:x); }
  public static void Streak(string path,int w,int h){
    var bm=new Bitmap(w,h,PixelFormat.Format32bppArgb);
    for(int y=0;y<h;y++) for(int x=0;x<w;x++){
      double u=(x+0.5)/w*2-1, v=(y+0.5)/h;              // u: -1..1 横, v: 0(上)..1(下)
      double across=Math.Exp(-u*u/0.12);                 // 横は細いガウス
      double tail=Math.Pow(C01(v/0.75),1.6);             // 上は薄く、下（落ちる先）ほど濃い
      double head=C01((1-v)/0.08);                       // 下端は短く切る
      double edge=C01((1-Math.Abs(u))/0.15);             // 左右の端で 0
      double a=across*tail*head*edge;
      bm.SetPixel(x,y,Color.FromArgb((int)(C01(a)*255),255,255,255));
    }
    bm.Save(path,ImageFormat.Png); bm.Dispose();
  }
}
"@
$tex = Join-Path $OutDir "Texture"; New-Item -ItemType Directory -Force $tex | Out-Null
[RainTex]::Streak((Join-Path $tex $Name), 32, 256)
"done: $tex\$Name"
