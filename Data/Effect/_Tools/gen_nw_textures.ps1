param([string]$OutDir)
New-Item -ItemType Directory -Force $OutDir | Out-Null
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @"
using System;
using System.Drawing;
using System.Drawing.Imaging;
public static class NW {
  static double Clamp(double v){ return v<0?0:(v>1?1:v); }
  static double Smooth(double e0,double e1,double x){ double t=Clamp((x-e0)/(e1-e0)); return t*t*(3-2*t); }
  // layers: weight + rgb. rgb = weighted hue, alpha = sum of weights (premultiplied-style for additive)
  static Color Mix(double[] w, int[][] c){
    double sw=0,r=0,g=0,b=0; for(int i=0;i<w.Length;i++){ if(w[i]<=0) continue; sw+=w[i]; r+=w[i]*c[i][0]; g+=w[i]*c[i][1]; b+=w[i]*c[i][2]; }
    if(sw<1e-4) return Color.FromArgb(0,0,0,0);
    return Color.FromArgb((int)(255*Clamp(sw)), (int)Math.Min(255,r/sw), (int)Math.Min(255,g/sw), (int)Math.Min(255,b/sw));
  }
  static readonly int[] WHITE={255,255,255}, GREEN={30,255,120}, PALE={190,255,215};
  static double Hash(int i){ double s=Math.Sin(i*127.1+311.7)*43758.5453; return s-Math.Floor(s); }
  static double Noise1(double x){ int i=(int)Math.Floor(x); double f=x-i; double u=f*f*(3-2*f); return Hash(i)*(1-u)+Hash(i+1)*u; }
  static double Wrap(double a,int n){ // periodic 1D noise over angle, n cells
    double x=(a/(2*Math.PI)+0.5)*n; int i=(int)Math.Floor(x); double f=x-i; double u=f*f*(3-2*f);
    return Hash(((i%n)+n)%n)*(1-u)+Hash((((i+1)%n)+n)%n)*u; }
  public delegate Color Px(double x,double y);
  public static void Save(string path,int w,int h,Px fn){
    var bmp=new Bitmap(w,h,PixelFormat.Format32bppArgb);
    for(int y=0;y<h;y++) for(int x=0;x<w;x++){ double nx=(x+0.5)/w*2-1, ny=(y+0.5)/h*2-1; bmp.SetPixel(x,y,fn(nx,ny)); }
    bmp.Save(path,ImageFormat.Png); bmp.Dispose();
  }
  // Diamond: tall sharp rhombus, white core -> green rim, plus a short horizontal glint. Baked color.
  public static Color Diamond(double x,double y){
    double d=Math.Abs(x)/0.42+Math.Abs(y)/0.98;           // 1 at diamond edge
    double r=Math.Sqrt(x*x+y*y);
    double body=1-Smooth(0.72,1.0,d);                       // crisp edge
    double core=1-Smooth(0.05,0.55,d);
    double glow=Math.Exp(-r*r/0.10)*0.35*(1-Smooth(0.85,1.0,r));
    double glint=Math.Exp(-y*y/0.0012)*Math.Pow(Clamp(1-Math.Abs(x)/0.7),2)*0.7;
    double rim=body*(1-core);
    return Mix(new double[]{core, rim*0.95, glow*(1-body), glint*(1-core)}, new int[][]{WHITE,GREEN,GREEN,PALE});
  }
  // Cross: 4-point sharp star (white, tint by node)
  public static Color Cross(double x,double y){
    double r=Math.Sqrt(x*x+y*y);
    double rh=Math.Exp(-y*y/0.0009)*Math.Pow(Clamp(1-Math.Abs(x)),3);
    double rv=Math.Exp(-x*x/0.0009)*Math.Pow(Clamp(1-Math.Abs(y)),3);
    double dg=Math.Exp(-Math.Pow(Math.Abs(x)-Math.Abs(y),2)/0.0006)*Math.Pow(Clamp(1-r/0.55),3)*0.45;
    double core=Math.Exp(-r*r/0.012);
    double glow=Math.Exp(-r*r/0.08)*0.35*(1-Smooth(0.8,1.0,r));
    return Mix(new double[]{Math.Max(Math.Max(rh,rv),dg)+core+glow}, new int[][]{WHITE});
  }
  // Rays: radial burst, 18 spikes of varied length (white)
  public static Color Rays(double x,double y){
    double r=Math.Sqrt(x*x+y*y), a=Math.Atan2(y,x); int n=18;
    double best=0;
    for(int i=0;i<n;i++){ double ang=-Math.PI+(i+0.5)*2*Math.PI/n+ (Hash(i+7)-0.5)*0.25; double len=0.45+0.53*Hash(i+3); double wid=0.035+0.04*Hash(i+11);
      double da=Math.Atan2(Math.Sin(a-ang),Math.Cos(a-ang)); double t=Clamp(1-r/len); double v=Math.Exp(-da*da/(wid*wid*t*t+1e-5))*Math.Pow(t,1.5); if(v>best) best=v; }
    double core=Math.Exp(-r*r/0.02);
    return Mix(new double[]{best+core}, new int[][]{WHITE});
  }
  // Ring: crisp ring + soft inner glow (white)
  public static Color Ring(double x,double y){
    double r=Math.Sqrt(x*x+y*y);
    double ring=Math.Exp(-Math.Pow((r-0.86)/0.028,2));
    double inner=Smooth(0.35,0.86,r)*(1-Smooth(0.86,0.9,r))*0.22;
    return Mix(new double[]{ring+inner}, new int[][]{WHITE});
  }
  // Plasma: jagged starburst blob with hollow-ish bright edge (white)
  public static Color Plasma(double x,double y){
    double r=Math.Sqrt(x*x+y*y), a=Math.Atan2(y,x);
    double edge=0.5+0.3*Math.Pow(Wrap(a,11),2)+0.16*Wrap(a+1.3,29);
    double q=r/edge;
    double body=(1-Smooth(0.9,1.0,q))*(0.12+0.88*Math.Pow(Smooth(0.25,0.97,q),2));
    double wisp=Math.Pow(Clamp(1-q),0.5)*0.0;
    return Mix(new double[]{body+wisp}, new int[][]{WHITE});
  }
  // Spark: sharp spindle (white). Long along Y.
  public static Color Spark(double x,double y){
    double w=0.9*Math.Pow(Clamp(1-Math.Abs(y)),1.2);
    double v=Math.Exp(-x*x/(w*w*0.12+1e-5))*Math.Pow(Clamp(1-Math.Abs(y)),0.7);
    return Mix(new double[]{v}, new int[][]{WHITE});
  }
  // Trail profile across X: white center line, vivid green rim (baked)
  public static Color Trail(double x,double y){
    double ax=Math.Abs(x);
    double core=Math.Exp(-x*x/0.012);
    double rim=(1-Smooth(0.7,1.0,ax))*0.75*(1-core);
    return Mix(new double[]{core,rim}, new int[][]{WHITE,GREEN});
  }
  // Blades: two opposite crescents tapering (white)
  public static Color Blades(double x,double y){
    double r=Math.Sqrt(x*x+y*y), a=Math.Atan2(y,x); double best=0;
    for(int k=0;k<2;k++){ double a0=k*Math.PI; double da=a-a0; da=Math.Atan2(Math.Sin(da),Math.Cos(da)); // 0..2.2 rad sweep
      if(da<0||da>2.3) continue; double t=da/2.3; double rad=0.88-0.12*t; double wid=0.11*Math.Pow(1-t,1.3)+0.004;
      double v=Math.Exp(-Math.Pow((r-rad)/wid,2))*Math.Pow(1-t,0.6)*Smooth(0.0,0.12,t); if(v>best) best=v; }
    return Mix(new double[]{best}, new int[][]{WHITE});
  }
  // Spray: upward fan sheet with streaks (white). Origin at bottom center.
  public static Color Spray(double x,double y){
    double py=(1-y)/2; // 0 bottom .. 1 top
    double spread=0.12+0.75*py; double ax=Math.Abs(x)/spread;
    double sheet=(1-Smooth(0.6,1.0,ax))*Smooth(0.0,0.12,py)*(1-Smooth(0.55,1.0,py));
    double streak=0.55+0.45*Math.Pow(Math.Abs(Math.Sin(x/spread*9.0+1.7)),3);
    return Mix(new double[]{sheet*streak}, new int[][]{WHITE});
  }
  public static void Make(string dir,string name,int w,int h){ Px f=null; switch(name){case "Diamond":f=Diamond;break;case "Cross":f=Cross;break;case "Rays":f=Rays;break;case "Ring":f=Ring;break;case "Plasma":f=Plasma;break;case "Spark":f=Spark;break;case "Trail":f=Trail;break;case "Blades":f=Blades;break;case "Spray":f=Spray;break;case "Soft":f=Soft;break;case "Orb":f=Orb;break;case "Streak":f=Streak;break;} Save(dir+"\\NW_"+name+".png",w,h,f); }
  // Orb: crisp bright rim + soft fresnel interior (white). Clean energy sphere silhouette.
  public static Color Orb(double x,double y){
    double r=Math.Sqrt(x*x+y*y);
    double rim=Math.Exp(-Math.Pow((r-0.84)/0.045,2));
    double inner=Math.Pow(Smooth(0.0,0.84,r),3)*0.55*(1-Smooth(0.84,0.87,r));
    double core=0.12*(1-Smooth(0.0,0.84,r));
    double halo=Math.Exp(-Math.Pow((r-0.84)/0.13,2))*0.25*(1-Smooth(0.92,1.0,r));
    return Mix(new double[]{rim+inner+core+halo}, new int[][]{WHITE});
  }
  public static Color Soft(double x,double y){ double r=Math.Sqrt(x*x+y*y); return Mix(new double[]{Math.Exp(-r*r/0.18)*(1-Smooth(0.75,1.0,r))}, new int[][]{WHITE}); }
  public static Color Streak(double x,double y){ double v=Math.Exp(-y*y/0.05)*Math.Pow(Clamp(1-Math.Abs(x)),2.5); return Mix(new double[]{v}, new int[][]{WHITE}); }
}
"@
$o=$OutDir
[NW]::Make($o,"Diamond",128,128)
[NW]::Make($o,"Cross",128,128)
[NW]::Make($o,"Rays",256,256)
[NW]::Make($o,"Ring",256,256)
[NW]::Make($o,"Plasma",256,256)
[NW]::Make($o,"Spark",32,128)
[NW]::Make($o,"Trail",64,16)
[NW]::Make($o,"Blades",256,256)
[NW]::Make($o,"Spray",128,128)
[NW]::Make($o,"Soft",64,64)
[NW]::Make($o,"Orb",256,256)
[NW]::Make($o,"Streak",128,16)
"done"



