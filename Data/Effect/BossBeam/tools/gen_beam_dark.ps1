# BossBeam の暗い層の素材を作る（2026-11-11）
#   Texture/BB_TubeSpiral.png : 筒に巻く螺旋の膜（U=周、V=長さ。上下左右とも継ぎ目なし）
#   Texture/BB_Vortex.png     : 先端の渦（中心は抜き、縁で alpha=0）
#   Model/BB_Tube.efkmodel    : 中空の筒（Z軸方向、半径 R、長さ ±H。両端は頂点 alpha で薄く）
# 使い方: powershell -NoProfile -ExecutionPolicy Bypass -File gen_beam_dark.ps1 -OutDir <BossBeamフォルダ>
param([string]$OutDir = ".", [string]$TubeTex = "BB_TubeSpiral.png", [string]$VortexTex = "BB_Vortex.png", [string]$ModelName = "BB_Tube.efkmodel",
      [double]$Radius = 0.66, [double]$HalfLen = 1.1, [double]$EndFade = 0.2)

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @"
using System; using System.Drawing; using System.Drawing.Imaging; using System.IO;
public static class BeamDark {
  static double C01(double x){ return x<0?0:(x>1?1:x); }
  static double Frac(double x){ return x-Math.Floor(x); }
  static double SStep(double a,double b,double x){ double t=C01((x-a)/(b-a)); return t*t*(3-2*t); }
  // 周期ノイズ（整数周波数の sin の和 → 継ぎ目なし）
  static double PNoise(double u,double v,int seed){
    double s=0; var r=new Random(seed);
    for(int i=0;i<5;i++){ int fu=r.Next(1,6), fv=r.Next(1,4); double p=r.NextDouble()*6.283;
      s+=Math.Sin(6.2832*(fu*u+fv*v)+p)/(1+i*0.6); }
    return s/2.6;
  }
  static void Put(Bitmap bm,int x,int y,double r,double g,double b,double a){
    bm.SetPixel(x,y,Color.FromArgb((int)(C01(a)*255),(int)(C01(r)*255),(int)(C01(g)*255),(int)(C01(b)*255)));
  }
  // 螺旋の膜: arms 本の帯が V 1 周期で 1 ピッチ。帯の中は濃い紫、帯の縁だけ明るい紫、帯の間は薄い膜
  public static void Tube(string path,int size,int arms){
    var bm=new Bitmap(size,size,PixelFormat.Format32bppArgb);
    for(int y=0;y<size;y++) for(int x=0;x<size;x++){
      double u=(x+0.5)/size, v=(y+0.5)/size;
      double n=PNoise(u,v,7)*0.10 + PNoise(u*2,v*2,11)*0.05;      // 帯の縁の乱れ（電気っぽさ）
      double ph=Frac(arms*u + v + n);                                // 螺旋の位相
      double d=Math.Abs(ph-0.5)*2;                                   // 0=帯の中心, 1=帯の間
      double band=1-SStep(0.46,0.54,d);                              // 帯（幅は位相の約半分）
      double edge=Math.Exp(-Math.Pow((d-0.50)/0.06,2));              // 帯の両縁（明るい線）
      double fil=0.5+0.5*Math.Sin(6.2832*(arms*2*u - 3*v) + n*8);   // 帯の中の細かい筋
      double a=0.22 + 0.72*band*(0.88+0.12*fil);                     // 膜 0.22 / 帯 0.85〜0.94
      a=Math.Max(a, 0.96*edge);
      double dr=0.13, dg=0.02, db=0.24;                              // 暗い紫（33,5,61）
      double lr=0.86, lg=0.66, lb=1.0;                               // 縁の明るい紫（220,168,255）
      double e=C01(edge*1.15);
      Put(bm,x,y, dr+(lr-dr)*e, dg+(lg-dg)*e, db+(lb-db)*e, a);
    }
    bm.Save(path,ImageFormat.Png); bm.Dispose();
  }
  // 先端の渦: arms 本の対数螺旋の腕。中心 r<0.18 は抜き、r>0.46 で alpha=0
  public static void Vortex(string path,int size,int arms){
    var bm=new Bitmap(size,size,PixelFormat.Format32bppArgb);
    for(int y=0;y<size;y++) for(int x=0;x<size;x++){
      double px=(x+0.5)/size-0.5, py=(y+0.5)/size-0.5;
      double r=Math.Sqrt(px*px+py*py), th=Math.Atan2(py,px)/6.2832;
      double n=PNoise(th,r*2,23)*0.08;
      double ph=Frac(arms*th + 2.2*Math.Log(Math.Max(r,1e-3)/0.18) + n);
      double d=Math.Abs(ph-0.5)*2;
      double band=1-SStep(0.45,0.62,d);
      double edge=Math.Exp(-Math.Pow((d-0.54)/0.05,2));
      double radial=SStep(0.16,0.24,r)*(1-SStep(0.36,0.46,r));
      double a=(0.30+0.62*band)*radial; a=Math.Max(a,0.9*edge*radial);
      double e=C01(edge*1.1);
      Put(bm,x,y, 0.16+(0.72-0.16)*e, 0.035+(0.45-0.035)*e, 0.28+(1.0-0.28)*e, a);
    }
    bm.Save(path,ImageFormat.Png); bm.Dispose();
  }
  // .efkmodel v5: 中空の筒（外面のみ。描画は両面 Culling=Double）
  public static void TubeModel(string path,double R,double H,double endFade,int seg,int rings){
    var ms=new MemoryStream(); var w=new BinaryWriter(ms);
    w.Write(5); w.Write(1.0f); w.Write(1); w.Write(1);
    int vc=(seg+1)*(rings+1); w.Write(vc);
    for(int j=0;j<=rings;j++){
      double t=(double)j/rings, z=-H+2*H*t;
      double dist=Math.Min(z+H, H-z);                       // 端からの距離
      byte va=(byte)(255*C01(dist/endFade));
      for(int i=0;i<=seg;i++){
        double u=(double)i/seg, a=u*6.2832, cx=Math.Cos(a), sy=Math.Sin(a);
        w.Write((float)(R*cx)); w.Write((float)(R*sy)); w.Write((float)z);   // pos
        w.Write((float)cx); w.Write((float)sy); w.Write(0f);                 // normal
        w.Write(0f); w.Write(0f); w.Write(1f);                               // binormal
        w.Write((float)(-sy)); w.Write((float)cx); w.Write(0f);              // tangent
        w.Write((float)u); w.Write((float)t);                                // uv
        w.Write((byte)255); w.Write((byte)255); w.Write((byte)255); w.Write(va);
      }
    }
    w.Write(seg*rings*2);
    for(int j=0;j<rings;j++) for(int i=0;i<seg;i++){
      int a=j*(seg+1)+i, b=a+1, c=a+(seg+1), d=c+1;
      w.Write(a); w.Write(c); w.Write(b);  w.Write(b); w.Write(c); w.Write(d);
    }
    w.Flush(); File.WriteAllBytes(path, ms.ToArray());
  }
}
"@

$tex = Join-Path $OutDir "Texture"; $mdl = Join-Path $OutDir "Model"
New-Item -ItemType Directory -Force $tex, $mdl | Out-Null
[BeamDark]::Tube((Join-Path $tex $TubeTex), 256, 3)
[BeamDark]::Vortex((Join-Path $tex $VortexTex), 256, 3)
[BeamDark]::TubeModel((Join-Path $mdl $ModelName), $Radius, $HalfLen, $EndFade, 32, 6)
"done: $tex\$TubeTex, $tex\$VortexTex, $mdl\$ModelName"
