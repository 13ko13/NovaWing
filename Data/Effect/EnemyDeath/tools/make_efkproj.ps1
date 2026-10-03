# Builds EnemyDeath_vN.efkproj (hand-written Effekseer XML). Enum-only settings that are
# easier to set by name (easing type, flipbook, billboard) are applied afterwards via the MCP.
# Usage: powershell -NoProfile -ExecutionPolicy Bypass -File make_efkproj.ps1 -Out <path.efkproj>
param([string]$Out = (Join-Path (Split-Path -Parent $PSScriptRoot) "EnemyDeath_v1.efkproj"))

$inv = [System.Globalization.CultureInfo]::InvariantCulture
function Fmt($v) { ([double]$v).ToString("0.#####", $inv) }
# random value: R center amp  /  fixed: R c
function RandV($c, $a = 0) { "<Center>$(Fmt $c)</Center><Max>$(Fmt ($c + $a))</Max><Min>$(Fmt ($c - $a))</Min>" }
function V3($tag, $x, $y, $z) { "<$tag><X>$x</X><Y>$y</Y><Z>$z</Z></$tag>" }
function Col($r, $g, $b, $a) { "<R>$(RandV $r)</R><G>$(RandV $g)</G><B>$(RandV $b)</B><A>$(RandV $a)</A>" }
function ColEase($s, $e) { "<ColorAll><Type>2</Type><Easing><Start>$(Col @s)</Start><End>$(Col @e)</End></Easing></ColorAll>" }

function Common($count, $life, $lifeAmp = 0, $offset = 0, $interval = 1E-05) {
    $gen = if ($count -eq "inf") { "<Infinite>True</Infinite>" } else { "<Value>$count</Value>" }
    $gt = "<Generation><GenerationTime>$(RandV $interval)</GenerationTime>" + $(if ($offset) { "<GenerationTimeOffset>$(RandV $offset)</GenerationTimeOffset>" } else { "" }) + "</Generation>"
    "<CommonValues><MaxGeneration>$gen</MaxGeneration><RotationEffectType>1</RotationEffectType><ScaleEffectType>1</ScaleEffectType><Life>$(RandV $life $lifeAmp)</Life>$gt$extra</CommonValues>"
}
function PVA($loc, $vel, $acc) {
    # each is @(cx,ax, cy,ay, cz,az)
    function Tri($tag, $v) { if (-not $v) { return "" }; "<$tag><X>$(RandV $v[0] $v[1])</X><Y>$(RandV $v[2] $v[3])</Y><Z>$(RandV $v[4] $v[5])</Z></$tag>" }
    "<LocationValues><Type>1</Type><PVA>$(Tri 'Location' $loc)$(Tri 'Velocity' $vel)$(Tri 'Acceleration' $acc)</PVA></LocationValues>"
}
function RotRand($velAmp) {
    "<RotationValues><Type>1</Type><PVA><Rotation><Z>$(RandV 180 180)</Z></Rotation><Velocity><Z>$(RandV 0 $velAmp)</Z></Velocity></PVA></RotationValues>"
}
# easing type codes (as stored in XML): 21 = EaseOutCubic, 31 = EaseOutQuartic
function ScaleEase($s, $sa, $e, $ea, $type = 21) {
    "<ScalingValues><Type>4</Type><SingleEasing><Start>$(RandV $s $sa)</Start><End>$(RandV $e $ea)</End><Type>$type</Type></SingleEasing></ScalingValues>"
}
# uv: "" | flipbook "FB:<cell>:<cols>:<rows>:<frameLength>" | random still sheet "SHEET:<cell>:<cols>:<rows>"
function UVXml($uv) {
    if (-not $uv) { return "" }
    $p = $uv.Split(":")
    $size = "<Size><X>$($p[1])</X><Y>$($p[1])</Y></Size><FrameCountX>$($p[2])</FrameCountX><FrameCountY>$($p[3])</FrameCountY>"
    if ($p[0] -eq "FB") {
        return "<UV>2</UV><UVAnimation><AnimationParams>$size<FrameLength><Value>$($p[4])</Value></FrameLength></AnimationParams><FlipbookInterpolationType>1</FlipbookInterpolationType></UVAnimation>"
    }
    $last = [int]$p[2] * [int]$p[3] - 1
    "<UV>2</UV><UVAnimation><AnimationParams>$size<FrameLength><Infinite>True</Infinite></FrameLength><StartSheet><Center>$($last / 2)</Center><Max>$last</Max><Min>0</Min></StartSheet></AnimationParams></UVAnimation>"
}
function Rend($tex, $blend, $fadeIn = 0, $fadeOut = 0, $uv = "") {
    $s = "<RendererCommonValues><ColorTexture>Texture/$tex</ColorTexture><AlphaBlend>$blend</AlphaBlend>"
    if ($fadeIn) { $s += "<FadeInType>1</FadeInType><FadeIn><Frame>$fadeIn</Frame></FadeIn>" }
    if ($fadeOut) { $s += "<FadeOutType>1</FadeOutType><FadeOut><Frame>$fadeOut</Frame></FadeOut>" }
    $s + (UVXml $uv) + "</RendererCommonValues>"
}
function Node($name, $body, $children = "") {
    "<Node>$body<Name>$name</Name><Children>$children</Children></Node>`n"
}
$BLEND = 1; $ADD = 2
$FIRE = "FB:256:4:4:2"; $SMOKE = "SHEET:256:2:2"; $DEBRIS = "SHEET:128:2:2"

$n = ""

# --- lingering dark smoke (behind everything, appears as the fire cools) ---
$n += Node "Smoke" ((Common 8 80 15 10) + (PVA @(0,1.2, 0.2,0.8, 0,1.2) @(0,0.02, 0.014,0.008, 0,0.02) $null) +
    (RotRand 0.6) + (ScaleEase 3.6 0.6 7.5 1.0) + (Rend "ED_Smoke.png" $BLEND 12 35 $SMOKE) +
    "<DrawingValues>$(ColEase @(62,54,50,225) @(46,46,50,0))</DrawingValues>")

# --- big additive heat glow ---
$n += Node "HeatGlow" ((Common 1 28) + (ScaleEase 7 0 12 0) + (Rend "ED_Glow.png" $ADD) +
    "<DrawingValues>$(ColEase @(255,160,70,235) @(200,45,5,0))</DrawingValues>")

# --- main fireball puffs (flipbook) ---
$n += Node "Fireball" ((Common 8 32 4) + (PVA @(0,0.6, 0,0.6, 0,0.6) @(0,0.05, 0.01,0.04, 0,0.05) $null) +
    (RotRand 1.5) + (ScaleEase 3.4 0.5 6.2 0.8) + (Rend "ED_Fire.png" $BLEND 0 6 $FIRE) +
    "<DrawingValues>$(ColEase @(255,255,255,255) @(255,255,255,255))</DrawingValues>")

# --- secondary bursts, a little later and further out ---
$n += Node "Fireball2" ((Common 6 28 3 4) + (PVA @(0,1.1, 0.2,1.0, 0,1.1) @(0,0.04, 0.012,0.03, 0,0.04) $null) +
    (RotRand 2) + (ScaleEase 2.4 0.4 4.4 0.6) + (Rend "ED_Fire2.png" $BLEND 0 6 $FIRE) +
    "<DrawingValues>$(ColEase @(255,255,255,255) @(255,255,255,255))</DrawingValues>")

# --- additive fire on top for the hot phase only ---
$n += Node "FireBloom" ((Common 4 14 2) + (PVA @(0,0.5, 0,0.5, 0,0.5) @(0,0.04, 0,0.04, 0,0.04) $null) +
    (RotRand 2) + (ScaleEase 3.6 0.5 6.0 0.6) + (Rend "ED_Fire.png" $ADD 0 0 "FB:256:4:4:1") +
    "<DrawingValues>$(ColEase @(255,210,140,130) @(255,90,20,0))</DrawingValues>")

# --- fire licking inside the smoke after the main ball cools ---
$n += Node "InnerFire" ((Common 6 18 4 9 2.5) + (PVA @(0,1.1, 0.2,0.9, 0,1.1) @(0,0.02, 0.02,0.01, 0,0.02) $null) +
    (RotRand 3) + (ScaleEase 2.2 0.5 3.8 0.6) + (Rend "ED_Fire2.png" $ADD 3 0 "FB:256:4:4:1") +
    "<DrawingValues>$(ColEase @(255,190,110,170) @(255,70,10,0))</DrawingValues>")

# --- shockwave ---
$n += Node "Shockwave" ((Common 1 12) + (ScaleEase 1.0 0 11 0 31) + (Rend "ED_Ring.png" $ADD) +
    "<DrawingValues>$(ColEase @(255,225,180,140) @(255,130,50,0))</DrawingValues>")

# --- sparks (directional billboard: stretched along velocity) ---
$n += Node "Sparks" ((Common 36 20 6) + (PVA $null @(0,0.5, 0.08,0.45, 0,0.5) @(0,0, -0.012,0, 0,0)) +
    "<ScalingValues><Type>2</Type><Easing><Start><X>$(RandV 0.22)</X><Y>$(RandV 2.4 0.7)</Y></Start><End><X>$(RandV 0.1)</X><Y>$(RandV 0.6)</Y></End></Easing></ScalingValues>" +
    (Rend "ED_Spark.png" $ADD) +
    "<DrawingValues>$(ColEase @(255,250,220,255) @(255,90,10,0))<Sprite><Billboard>4</Billboard></Sprite></DrawingValues>")

# --- debris chunks with smoke trails ---
$trail = Node "DebrisTrail" ("<CommonValues><MaxGeneration><Infinite>True</Infinite></MaxGeneration><LocationEffectType>1</LocationEffectType><RotationEffectType>1</RotationEffectType><ScaleEffectType>1</ScaleEffectType><Life>$(RandV 18 3)</Life><Generation><GenerationTime>$(RandV 1)</GenerationTime><GenerationTimeOffset>$(RandV 5)</GenerationTimeOffset></Generation></CommonValues>" +
    (PVA $null @(0,0.004, 0.006,0.003, 0,0.004) $null) + (RotRand 2) + (ScaleEase 0.45 0.08 1.4 0.25) + (Rend "ED_Smoke.png" $BLEND 0 0 $SMOKE) +
    "<DrawingValues>$(ColEase @(235,120,50,235) @(38,36,36,0))</DrawingValues>")
$burn = Node "DebrisBurn" ((Common 1 40) + (ScaleEase 1.3 0 0.4 0) + (Rend "ED_Glow.png" $ADD) +
    "<DrawingValues>$(ColEase @(255,170,70,220) @(255,70,10,0))</DrawingValues>")
$n += Node "Debris" ((Common 8 42 10) + (PVA @(0,0.5, 0,0.5, 0,0.5) @(0,0.17, 0.1,0.12, 0,0.17) @(0,0, -0.007,0, 0,0)) +
    (RotRand 14) + (ScaleEase 0.5 0.15 0.5 0.15) + (Rend "ED_Debris.png" $BLEND 0 10 $DEBRIS) +
    "<DrawingValues>$(ColEase @(255,255,255,255) @(170,170,170,255))</DrawingValues>") ($trail + $burn)

# --- floating embers ---
$n += Node "Embers" ((Common 18 55 15 3) + (PVA @(0,1.2, 0,1.2, 0,1.2) @(0,0.07, 0.03,0.05, 0,0.07) @(0,0, -0.0012,0, 0,0)) +
    (ScaleEase 0.18 0.05 0.04 0) + (Rend "ED_Glow.png" $ADD) +
    "<DrawingValues>$(ColEase @(255,220,120,255) @(255,60,0,0))</DrawingValues>")

# --- initial flash (front) ---
$n += Node "Flash" ((Common 1 8) + "<RotationValues><Type>1</Type><PVA><Rotation><Z>$(RandV 180 180)</Z></Rotation></PVA></RotationValues>" +
    (ScaleEase 9 0 5 0) + (Rend "ED_Flash.png" $ADD) +
    "<DrawingValues>$(ColEase @(255,250,235,255) @(255,200,120,0))</DrawingValues>")
$n += Node "CoreFlash" ((Common 1 6) + (ScaleEase 5 0 3 0) + (Rend "ED_Glow.png" $ADD) +
    "<DrawingValues>$(ColEase @(255,255,245,255) @(255,220,150,0))</DrawingValues>")

$root = Node "EnemyDeath" ("<CommonValues><MaxGeneration><Value>1</Value></MaxGeneration><Life>$(RandV 120)</Life></CommonValues><DrawingValues><Type>0</Type></DrawingValues>") $n

$xml = @"
<?xml version="1.0" encoding="utf-8"?>
<EffekseerProject>
  <Root>
    <Name>Root</Name>
    <Children>
$root
    </Children>
  </Root>
  <ToolVersion>1.80.7</ToolVersion>
  <Version>3</Version>
  <StartFrame>0</StartFrame>
  <EndFrame>110</EndFrame>
  <IsLoop>False</IsLoop>
</EffekseerProject>
"@
[System.IO.File]::WriteAllText($Out, $xml, (New-Object System.Text.UTF8Encoding $false))
"wrote $Out"
