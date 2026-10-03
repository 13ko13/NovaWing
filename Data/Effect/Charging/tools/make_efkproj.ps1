# Builds Charging_vN.efkproj and PlayerChargeBullet_vN.efkproj (hand-written Effekseer XML).
# Both effects share the textures from gen_charge_tex.ps1 and the same node "parts" below,
# so the charge-up and the shot read as one family (the orb you charged is the orb that flies).
# Usage: powershell -NoProfile -ExecutionPolicy Bypass -File make_efkproj.ps1 -Version 1 [-PreviewMove]
#   -PreviewMove : gives the bullet root a +Z velocity so the trail can be checked in the editor.
#                  Never export with it: in game the code moves the effect every frame.
param([int]$Version = 1, [switch]$PreviewMove)

$effectDir = Split-Path -Parent $PSScriptRoot          # ...\Data\Effect\Charging
$root = Split-Path -Parent $effectDir                   # ...\Data\Effect

$inv = [System.Globalization.CultureInfo]::InvariantCulture
function Fmt($v) { ([double]$v).ToString("0.#####", $inv) }
function RandV($c, $a = 0) { "<Center>$(Fmt $c)</Center><Max>$(Fmt ($c + $a))</Max><Min>$(Fmt ($c - $a))</Min>" }
function Col($r, $g, $b, $a) { "<R>$(RandV $r)</R><G>$(RandV $g)</G><B>$(RandV $b)</B><A>$(RandV $a)</A>" }
function ColEase($s, $e) { "<ColorAll><Type>2</Type><Easing><Start>$(Col @s)</Start><End>$(Col @e)</End></Easing></ColorAll>" }

# bind codes: 1 = WhenCreating (stays in the world), omitted = Always (follows the effect, incl. the code's SetScale)
function Common($count, $life, $lifeAmp = 0, $offset = 0, $interval = 1E-05, $intervalAmp = 0, [switch]$World) {
    $gen = if ($count -eq "inf") { "<Infinite>True</Infinite>" } else { "<Value>$count</Value>" }
    $gt = "<Generation><GenerationTime>$(RandV $interval $intervalAmp)</GenerationTime>" + $(if ($offset) { "<GenerationTimeOffset>$(RandV $offset)</GenerationTimeOffset>" } else { "" }) + "</Generation>"
    $bind = if ($World) { "<LocationEffectType>1</LocationEffectType>" } else { "" }
    "<CommonValues><MaxGeneration>$gen</MaxGeneration>$bind<Life>$(RandV $life $lifeAmp)</Life>$gt</CommonValues>"
}
function Tri($tag, $v) { if (-not $v) { return "" }; "<$tag><X>$(RandV $v[0] $v[1])</X><Y>$(RandV $v[2] $v[3])</Y><Z>$(RandV $v[4] $v[5])</Z></$tag>" }
function PVA($loc, $vel, $acc) { "<LocationValues><Type>1</Type><PVA>$(Tri 'Location' $loc)$(Tri 'Velocity' $vel)$(Tri 'Acceleration' $acc)</PVA></LocationValues>" }
# location easing: from a random box (half size $r) to the centre -> particles get sucked in
function LocInward($r, $type = 20) {
    "<LocationValues><Type>2</Type><Easing><Start><X>$(RandV 0 $r)</X><Y>$(RandV 0 $r)</Y><Z>$(RandV 0 $r)</Z></Start><End><X>$(RandV 0)</X><Y>$(RandV 0)</Y><Z>$(RandV 0)</Z></End><Type>$type</Type></Easing></LocationValues>"
}
# random start angle + angular velocity (deg/F) around the view axis
function Spin($vel, $velAmp = 0) {
    "<RotationValues><Type>1</Type><PVA><Rotation><Z>$(RandV 180 180)</Z></Rotation><Velocity><Z>$(RandV $vel $velAmp)</Z></Velocity></PVA></RotationValues>"
}
# easing type codes (as stored in XML): tens = curve, ones = In(0)/Out(1)/InOut(2). 20 = EaseInCubic, 21 = EaseOutCubic, 31 = EaseOutQuartic.
# The editor silently accepts invalid codes (e.g. 13) but the runtime asserts in Easing.h.
function ScaleEase($s, $sa, $e, $ea, $type = 21) {
    "<ScalingValues><Type>4</Type><SingleEasing><Start>$(RandV $s $sa)</Start><End>$(RandV $e $ea)</End><Type>$type</Type></SingleEasing></ScalingValues>"
}
function ScaleFix($s) { "<ScalingValues><Fixed><Scale><X>$(Fmt $s)</X><Y>$(Fmt $s)</Y><Z>$(Fmt $s)</Z></Scale></Fixed></ScalingValues>" }
function UVXml($uv) {
    if (-not $uv) { return "" }
    $p = $uv.Split(":")
    $size = "<Size><X>$($p[1])</X><Y>$($p[1])</Y></Size><FrameCountX>$($p[2])</FrameCountX><FrameCountY>$($p[3])</FrameCountY>"
    $last = [int]$p[2] * [int]$p[3] - 1
    $start = "<StartSheet><Center>$($last / 2)</Center><Max>$last</Max><Min>0</Min></StartSheet>"
    if ($p[0] -eq "FB") {
        return "<UV>2</UV><UVAnimation><AnimationParams>$size<LoopType>1</LoopType><FrameLength><Value>$($p[4])</Value></FrameLength>$start</AnimationParams><FlipbookInterpolationType>1</FlipbookInterpolationType></UVAnimation>"
    }
    "<UV>2</UV><UVAnimation><AnimationParams>$size<FrameLength><Infinite>True</Infinite></FrameLength>$start</AnimationParams></UVAnimation>"
}
function Rend($tex, $blend, $fadeIn = 0, $fadeOut = 0, $uv = "") {
    $s = "<RendererCommonValues><ColorTexture>Texture/$tex</ColorTexture><AlphaBlend>$blend</AlphaBlend>"
    if ($fadeIn) { $s += "<FadeInType>1</FadeInType><FadeIn><Frame>$fadeIn</Frame></FadeIn>" }
    if ($fadeOut) { $s += "<FadeOutType>1</FadeOutType><FadeOut><Frame>$fadeOut</Frame></FadeOut>" }
    $s + (UVXml $uv) + "</RendererCommonValues>"
}
# plain "Billboard" ignores the Z rotation, so spinning sprites must be RotatedBillboard (stored as 3)
function Node($name, $body, $children = "") {
    if ($body.Contains("<RotationValues>") -and -not $body.Contains("<Billboard>")) {
        $body = $body.Replace("</DrawingValues>", "<Sprite><Billboard>3</Billboard></Sprite></DrawingValues>")
    }
    "<Node>$body<Name>$name</Name><Children>$children</Children></Node>`n"
}
function Draw($colEase, $extra = "") { "<DrawingValues>$colEase$extra</DrawingValues>" }
$BLEND = 1; $ADD = 2
$WISP = "FB:256:4:4:2"; $ARC = "SHEET:256:2:2"
$DIR = "<Sprite><Billboard>4</Billboard></Sprite>"     # directional billboard (stretched along motion)
$FOREVER = 100000
# palette (same as PlayerBullet)
$G = @(30, 255, 110); $C = @(235, 255, 238)
function GreenC($a) { @($G[0], $G[1], $G[2], $a) }
function CoreC($a) { @($C[0], $C[1], $C[2], $a) }

function Proj($name, $children, $end) {
    $top = Node $name ("<CommonValues><MaxGeneration><Value>1</Value></MaxGeneration><Life>$(RandV $FOREVER)</Life></CommonValues>" + $script:rootLoc + "<DrawingValues><Type>0</Type></DrawingValues>") $children
@"
<?xml version="1.0" encoding="utf-8"?>
<EffekseerProject>
  <Root>
    <Name>Root</Name>
    <Children>
$top
    </Children>
  </Root>
$($script:dyn)
  <ToolVersion>1.80.7</ToolVersion>
  <Version>3</Version>
  <StartFrame>0</StartFrame>
  <EndFrame>$end</EndFrame>
  <IsLoop>True</IsLoop>
</EffekseerProject>
"@
}

# ================= Charging =================
# Build-up 0-20F (code: charge_comp_frame = 20), then a "charged" pulse and a stable, crackling orb.
$CH = 20
$n = ""
# dark backing (normal blend) so the orb keeps contrast on the bright teal sea, then the halo
$n += Node "BackingGrow" ((Common 1 $CH) + (ScaleEase 0.5 0 3.0 0) + (Rend "CS_Glow.png" $BLEND) + (Draw (ColEase @(0,40,25,40) @(0,40,25,90))))
$n += Node "Backing" ((Common 1 $FOREVER 0 $CH) + (ScaleFix 3.0) + (Rend "CS_Glow.png" $BLEND) + (Draw (ColEase @(0,40,25,90) @(0,40,25,90))))
$n += Node "HaloGrow" ((Common 1 $CH) + (ScaleEase 0.6 0 4.2 0) + (Rend "CS_Glow.png" $ADD) + (Draw (ColEase (GreenC 40) (GreenC 80))))
$n += Node "Halo" ((Common 1 $FOREVER 0 $CH) + (ScaleFix 4.2) + (Rend "CS_Glow.png" $ADD) + (Draw (ColEase (GreenC 80) (GreenC 80))))
# swirling plasma, contracting toward the core = energy being gathered
$n += Node "Wisp" ((Common "inf" 22 4 0 1.6 0.4) + (Spin 7 2) + (ScaleEase 2.8 0.3 0.9 0.1 20) +
    (Rend "CS_WispB.png" $ADD 6 8 $WISP) + (Draw (ColEase @(255,255,255,170) @(255,255,255,230))))
$n += Node "WispCharged" ((Common "inf" 18 3 $CH 3 0.5) + (Spin -9 2) + (ScaleEase 2.2 0.2 1.3 0.1) +
    (Rend "CS_Wisp2.png" $ADD 4 8 $WISP) + (Draw (ColEase @(255,255,255,140) @(255,255,255,170))))
# sparks sucked in from around
$inflowSpark = Node "Spark" ((Common 1 14 2) + (PVA @(0,0, 3.0,0.4, 0,0) @(0,0, -0.11,0.02, 0,0) @(0,0, -0.007,0, 0,0)) +
    "<ScalingValues><Type>2</Type><Easing><Start><X>$(RandV 0.10)</X><Y>$(RandV 0.6 0.15)</Y></Start><End><X>$(RandV 0.07)</X><Y>$(RandV 1.4 0.3)</Y></End></Easing></ScalingValues>" +
    (Rend "CS_Spark.png" $ADD 5 0) + (Draw (ColEase (GreenC 0) (CoreC 255)) $DIR))
$n += Node "Inflow" ((Common "inf" 15 0 0 1 0.4) + "<RotationValues><Type>1</Type><PVA><Rotation><X>$(RandV 0 180)</X><Y>$(RandV 0 180)</Y><Z>$(RandV 0 180)</Z></Rotation></PVA></RotationValues><DrawingValues><Type>0</Type></DrawingValues>") $inflowSpark
# crackling arcs on the orb surface (sparse while charging, frequent once charged)
$n += Node "Arc" ((Common "inf" 3 0 4 5 1.5) + (Spin 0) + (ScaleEase 1.4 0.2 1.6 0.2) + (Rend "CS_Arc.png" $ADD 0 0 $ARC) + (Draw (ColEase (CoreC 255) (GreenC 200))))
$n += Node "ArcCharged" ((Common "inf" 3 0 $CH 2.5 1) + (Spin 0) + (ScaleEase 1.7 0.3 2.0 0.3) + (Rend "CS_Arc.png" $ADD 0 0 $ARC) + (Draw (ColEase (CoreC 255) (GreenC 220))))
# core
$n += Node "CoreGrow" ((Common 1 $CH) + (ScaleEase 0.3 0 1.5 0 21) + (Rend "CS_Core.png" $ADD) + (Draw (ColEase @(255,255,255,200) @(255,255,255,255))))
$n += Node "Core" ((Common 1 $FOREVER 0 $CH) + (ScaleFix 1.5) + (Rend "CS_Core.png" $ADD) + (Draw (ColEase @(255,255,255,255) @(255,255,255,255))))
$n += Node "CoreShimmer" ((Common "inf" 6 1 $CH 3 1) + (ScaleEase 1.5 0 2.3 0.3) + (Rend "CS_Core.png" $ADD) + (Draw (ColEase @(255,255,255,110) @(255,255,255,0))))
$n += Node "StarGrow" ((Common 1 $CH) + (Spin 2) + (ScaleEase 0.6 0 3.2 0) + (Rend "CS_StarB.png" $ADD) + (Draw (ColEase (CoreC 150) (CoreC 230))))
$n += Node "Star" ((Common 1 $FOREVER 0 $CH) + (Spin 2) + (ScaleFix 3.2) + (Rend "CS_StarB.png" $ADD) + (Draw (ColEase (CoreC 230) (CoreC 230))))
# the moment it is charged (in sync with the ChargeComplete SE)
$n += Node "ChargedRing" ((Common 1 12 0 $CH) + (ScaleEase 1.0 0 4.5 0 31) + (Rend "CS_Ring.png" $ADD) + (Draw (ColEase (CoreC 255) (GreenC 0))))
$n += Node "ChargedFlash" ((Common 1 10 0 $CH) + (Spin 0) + (ScaleEase 6.0 0 2.0 0 31) + (Rend "CS_StarB.png" $ADD) + (Draw (ColEase (CoreC 255) (GreenC 0))))
$n += Node "ChargedSparks" ((Common 20 14 4 $CH) + (PVA $null @(0,0.22, 0,0.22, 0,0.22) $null) +
    "<ScalingValues><Type>2</Type><Easing><Start><X>$(RandV 0.08)</X><Y>$(RandV 1.1 0.3)</Y></Start><End><X>$(RandV 0.04)</X><Y>$(RandV 0.3)</Y></End></Easing></ScalingValues>" +
    (Rend "CS_Spark.png" $ADD) + (Draw (ColEase (CoreC 255) (GreenC 0)) $DIR))

$script:rootLoc = ""; $script:dyn = ""
$chOut = Join-Path $effectDir "Charging_v$Version.efkproj"
[System.IO.File]::WriteAllText($chOut, (Proj "Charging" $n 120), (New-Object System.Text.UTF8Encoding $false))
"wrote $chOut"

# ================= PlayerChargeBullet =================
$script:armScale = ""
if (Test-Path (Join-Path $PSScriptRoot "arm_scale_fcurve.xml")) { $script:armScale = [IO.File]::ReadAllText((Join-Path $PSScriptRoot "arm_scale_fcurve.xml")).Trim() }
$script:dyn = @"
  <Dynamic>
    <Inputs><DynamicInput><Input>0</Input></DynamicInput><DynamicInput><Input>0</Input></DynamicInput><DynamicInput><Input>0</Input></DynamicInput><DynamicInput><Input>0</Input></DynamicInput></Inputs>
    <Equations>
      <DynamicEquation><Name>PhantomGather</Name><Code>@O.x = @P.x * (1.0 - @In0)
@O.y = @P.y * (1.0 - @In0)
@O.z = @P.z * (1.0 - @In0)</Code></DynamicEquation>
      <DynamicEquation><Name>MergeSwell</Name><Code>@O.x = @P.x * @In0 * @In0
@O.y = @P.y * @In0 * @In0
@O.z = @P.z * @In0 * @In0</Code></DynamicEquation>
      <DynamicEquation><Name>PhantomOrbit</Name><Code>@O.x = @P.x * (1.0 - @In0) * cos(@GTime * 17.0)
@O.y = 0.0
@O.z = @P.x * (1.0 - @In0) * sin(@GTime * 17.0)</Code></DynamicEquation>
    </Equations>
  </Dynamic>
"@
# The same orb, now flying: tighter, faster swirl, and it sheds plasma that stays in the world.
$b = ""
# world-space wake (spawned every frame, left behind)
$b += Node "Trail" ((Common "inf" 16 0 0 1 0 -World) + (Rend "BulletTrail_v1b.png" $ADD 0 16) +
    "<DrawingValues>$(ColEase (GreenC 255) (GreenC 255))<Type>6</Type><Track><TrackSizeFor_Fixed>0.1</TrackSizeFor_Fixed><TrackSizeMiddle_Fixed>1.1</TrackSizeMiddle_Fixed><TrackSizeBack_Fixed>1.6</TrackSizeBack_Fixed></Track></DrawingValues>")
$b += Node "WakeWisp" ((Common "inf" 18 3 0 2 0.3 -World) + (PVA @(0,0.15, 0,0.15, 0,0.15) $null $null) + (Spin 5 3) + (ScaleEase 1.4 0.2 2.8 0.3) +
    (Rend "CS_Wisp2.png" $ADD 0 14 $WISP) + (Draw (ColEase @(255,255,255,170) @(255,255,255,60))))
$b += Node "WakeSpark" ((Common "inf" 14 4 0 2 0 -World) + (PVA $null @(0,0.05, 0,0.05, 0,0.05) $null) + (ScaleEase 0.25 0.08 0.05 0) +
    (Rend "CS_Glow.png" $ADD) + (Draw (ColEase (CoreC 255) (GreenC 0))))
# phantoms (分身): 5 small orbs orbiting the main one on random great circles, each drawing a light trail.
# Their orbit radius is multiplied by (1 - @In0); the code raises input 0 from 0 to 1 as the bullet nears
# its target, so they spiral in and merge into the orb right at the hit. Input 0 = 0 (default) keeps them out.
# Arm: random fixed orientation; its scale grows 0 -> 1 after launch (FCurve, frames) so they burst out.
$R = 2.0
function LocDyn($x, $eq) { "<LocationValues><Type>1</Type><PVA><Location><X>$(RandV $x)</X><Y>$(RandV 0)</Y><Z>$(RandV 0)</Z><DynamicEquationMin>$eq</DynamicEquationMin><DynamicEquationMax>$eq</DynamicEquationMax></Location></PVA></LocationValues>" }
function ScaleDyn($s, $eq) { "<ScalingValues><Type>1</Type><PVA><Scale><X>$(RandV $s)</X><Y>$(RandV $s)</Y><Z>$(RandV $s)</Z><DynamicEquationMin>$eq</DynamicEquationMin><DynamicEquationMax>$eq</DynamicEquationMax></Scale></PVA></ScalingValues>" }
# Orbit: children bound "WhenCreating" ignore a parent's rotation over time (verified with a fixed seed), so the
# orbit is computed in the equation "PhantomOrbit" instead: each frame a point is spawned at
# R x (1 - @In0) x (cos, sin)(@GTime [s] x 17)  (~16 deg/F) in the arm's randomly tilted plane. Points stay in the
# world, so with the bullet moving they form a helix that the Track draws; @In0 shrinks it into the orb.
# orbit position from the equation "PhantomOrbit" (x = R, the rest is computed): radius x (1 - @In0), angle from time
function OrbitGen() { LocDyn $R 2 }
$phTrail = Node "PhantomTrail" ((Common "inf" 12 0 0 1 0 -World) + (OrbitGen) + (Rend "BulletTrail_v1b.png" $ADD 0 12) +
    "<DrawingValues>$(ColEase (GreenC 255) (GreenC 255))<Type>6</Type><Track><TrackSizeFor_Fixed>0.04</TrackSizeFor_Fixed><TrackSizeMiddle_Fixed>0.4</TrackSizeMiddle_Fixed><TrackSizeBack_Fixed>0.65</TrackSizeBack_Fixed></Track></DrawingValues>")
$phHead = Node "PhantomHead" ((Common "inf" 1 0 0 1 0 -World) + (OrbitGen) + (ScaleFix 0.8) + (Rend "CS_Core.png" $ADD) + (Draw (ColEase @(255,255,255,255) @(255,255,255,255))))
$phStar = Node "PhantomStar" ((Common "inf" 1 0 0 1 0 -World) + (OrbitGen) + (Spin 0) + (ScaleFix 1.2) + (Rend "CS_StarB.png" $ADD) + (Draw (ColEase (CoreC 160) (CoreC 160))))
$b += Node "PhantomArm" ((Common 5 $FOREVER) + "<RotationValues><Type>1</Type><PVA><Rotation><X>$(RandV 0 180)</X><Y>$(RandV 0 180)</Y><Z>$(RandV 0 180)</Z></Rotation></PVA></RotationValues>" + $script:armScale + "<DrawingValues><Type>0</Type></DrawingValues>") ($phTrail + $phHead + $phStar)# the orb itself (follows the bullet)
$b += Node "Backing" ((Common 1 $FOREVER) + (ScaleFix 3.2) + (Rend "CS_Glow.png" $BLEND) + (Draw (ColEase @(0,40,25,90) @(0,40,25,90))))
$b += Node "Halo" ((Common 1 $FOREVER) + (ScaleFix 4.6) + (Rend "CS_Glow.png" $ADD) + (Draw (ColEase (GreenC 95) (GreenC 95))))
$b += Node "Wisp" ((Common "inf" 14 2 0 1.6 0.3) + (Spin 12 3) + (ScaleEase 2.4 0.2 1.4 0.1) +
    (Rend "CS_WispB.png" $ADD 3 6 $WISP) + (Draw (ColEase @(255,255,255,200) @(255,255,255,200))))
$b += Node "Wisp2" ((Common "inf" 12 2 1 2.5 0.3) + (Spin -14 3) + (ScaleEase 1.9 0.2 1.2 0.1) +
    (Rend "CS_Wisp2.png" $ADD 3 5 $WISP) + (Draw (ColEase @(255,255,255,170) @(255,255,255,170))))
$b += Node "Arc" ((Common "inf" 3 0 0 2 1) + (Spin 0) + (ScaleEase 1.8 0.3 2.1 0.3) + (Rend "CS_Arc.png" $ADD 0 0 $ARC) + (Draw (ColEase (CoreC 255) (GreenC 220))))
$b += Node "Core" ((Common 1 $FOREVER) + (ScaleFix 1.7) + (Rend "CS_Core.png" $ADD) + (Draw (ColEase @(255,255,255,255) @(255,255,255,255))))
$b += Node "CoreShimmer" ((Common "inf" 6 1 0 3 1) + (ScaleEase 1.7 0 2.6 0.3) + (Rend "CS_Core.png" $ADD) + (Draw (ColEase @(255,255,255,110) @(255,255,255,0))))
$b += Node "Star" ((Common 1 $FOREVER) + (Spin 5) + (ScaleFix 3.6) + (Rend "CS_StarB.png" $ADD) + (Draw (ColEase (CoreC 230) (CoreC 230))))
# swells as the phantoms merge (scale x @In0^2): the power concentrating right before the hit
$b += Node "GatherGlow" ((Common "inf" 2 0 0 1) + (ScaleDyn 7.5 1) + (Rend "CS_Glow.png" $ADD) + (Draw (ColEase (GreenC 150) (GreenC 150))))
$b += Node "GatherCore" ((Common "inf" 2 0 0 1) + (ScaleDyn 2.6 1) + (Rend "CS_Core.png" $ADD) + (Draw (ColEase @(255,255,255,255) @(255,255,255,255))))
# launch: the charged orb bursts out
$b += Node "LaunchRing" ((Common 1 12 0 0 1 0 -World) + (ScaleEase 1.2 0 4.5 0 31) + (Rend "CS_Ring.png" $ADD) + (Draw (ColEase (CoreC 255) (GreenC 0))))
$b += Node "LaunchFlash" ((Common 1 8 0 0 1 0 -World) + (Spin 0) + (ScaleEase 4.0 0 1.5 0 31) + (Rend "CS_StarB.png" $ADD) + (Draw (ColEase (CoreC 255) (GreenC 0))))

$script:rootLoc = if ($PreviewMove) { (PVA $null @(0,0, 0,0, 0.5,0) $null) } else { "" }
$suffix = if ($PreviewMove) { "_preview" } else { "" }
$bOut = Join-Path $root "PlayerChargeBullet\PlayerChargeBullet_v$Version$suffix.efkproj"
[System.IO.File]::WriteAllText($bOut, (Proj "ChargeBullet" $b 90), (New-Object System.Text.UTF8Encoding $false))
"wrote $bOut"
