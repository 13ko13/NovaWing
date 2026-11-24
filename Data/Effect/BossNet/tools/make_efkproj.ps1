# 網のエフェクトの編集用ファイル(.efkproj)を作る。数値はここで変える。
# 実行: powershell -NoProfile -ExecutionPolicy Bypass -File make_efkproj.ps1 -Effect Laser|Warning|Muzzle
# 出力: ../<Name>_gen.efkproj (エディタで開いて .efkefc に保存したら消す)
#
# 約束: 線の長手方向=ローカル+X、原点=始点。
# 動的パラメーター(エディタ単位。ゲーム単位÷50。読み込み倍率1.0の場合):
#   Laser  : @In0 = 伸びた割合(0〜1), @In1 = 線の長さ
#   Warning: @In0 = 線の長さ
#   Muzzle : なし
# 動的パラメーターは「生成した瞬間」にしか計算されないので、線は寿命1Fで毎フレーム出し直す。
param([Parameter(Mandatory = $true)][ValidateSet('Laser', 'Warning', 'Muzzle')][string]$Effect,
      [double]$TestLength = 20.0,  # 確認用の線の長さ(エディタ単位)。実物は140
      [double]$TestRate = 1.0,     # 確認用の伸びた割合
      [string]$Suffix = 'gen')

$outDir = Split-Path $PSScriptRoot -Parent

# ---- 部品 ----
function Rnd([double]$v) { "<Center>$v</Center><Max>$v</Max><Min>$v</Min>" }
function Vec3([double]$x, [double]$y, [double]$z, [int]$eq) {
    $s = "<X>$(Rnd $x)</X><Y>$(Rnd $y)</Y><Z>$(Rnd $z)</Z>"
    if ($eq -ge 0) { $s += "<DynamicEquationMin>$eq</DynamicEquationMin><DynamicEquationMax>$eq</DynamicEquationMax>" }
    $s
}
# 線の層(横に伸びる板)。locEq/scaleEq = 動的パラメーターの式番号(なければ-1)
function Node([string]$name, [string]$tex, [string]$blend, [int[]]$col, [string]$billboard,
              [double[]]$loc, [int]$locEq, [double[]]$scale, [int]$scaleEq, [double[]]$rot = @(0, 0, 0),
              [int]$life = 1, [int]$interval = 1, [double[]]$velocity = @(0, 0, 0)) {
    $blendNo = @{ Blend = 1; Add = 2 }[$blend]
    $bbNo = @{ Billboard = 0; Fixed = 2 }[$billboard]
    $infinite = '<MaxGeneration><Infinite>True</Infinite></MaxGeneration>'
    $gen = ''
    if ($interval -ne 1) { $gen = "<Generation><GenerationTime>$(Rnd $interval)</GenerationTime></Generation>" }
    @"
<Node>
<CommonValues>$infinite<Life>$(Rnd $life)</Life>$gen</CommonValues>
<LocationValues><Type>1</Type><PVA><Location>$(Vec3 $loc[0] $loc[1] $loc[2] $locEq)</Location></PVA></LocationValues>
<RotationValues><Type>0</Type><Fixed><Rotation><X>$($rot[0])</X><Y>$($rot[1])</Y><Z>$($rot[2])</Z></Rotation></Fixed></RotationValues>
<ScalingValues><Type>1</Type><PVA><Scale>$(Vec3 $scale[0] $scale[1] $scale[2] $scaleEq)</Scale></PVA></ScalingValues>
<RendererCommonValues><ColorTexture>Texture/$tex</ColorTexture><Wrap>1</Wrap><AlphaBlend>$blendNo</AlphaBlend></RendererCommonValues>
<DrawingValues><ColorAll><Fixed><R>$($col[0])</R><G>$($col[1])</G><B>$($col[2])</B><A>$($col[3])</A></Fixed></ColorAll><Sprite><Billboard>$bbNo</Billboard></Sprite></DrawingValues>
<Name>$name</Name>
<Children></Children>
</Node>
"@
}
function Equation([string]$name, [string]$code) {
    "<DynamicEquation><Name>$name</Name><Code>$code</Code></DynamicEquation>"
}
function Project([string[]]$nodes, [string[]]$equations, [double[]]$inputs, [int]$endFrame) {
    $inp = ($inputs | ForEach-Object { "<DynamicInput><Input>$_</Input></DynamicInput>" }) -join ''
    @"
<?xml version="1.0" encoding="utf-8"?>
<EffekseerProject>
<Root><Name>Root</Name><Children>
$($nodes -join "`n")
</Children></Root>
<Dynamic><Inputs>$inp</Inputs><Equations>
$($equations -join "`n")
</Equations></Dynamic>
<ProceduralModel><ProceduralModels></ProceduralModels></ProceduralModel>
<ToolVersion>1.80.6</ToolVersion><Version>3</Version>
<StartFrame>0</StartFrame><EndFrame>$endFrame</EndFrame><IsLoop>True</IsLoop>
</EffekseerProject>
"@
}

# ---- 数値(エディタ単位。1=ゲーム50ユニット) ----
$testLength = $TestLength
$testRate = $TestRate

switch ($Effect) {
    'Laser' {
        # 太さ(Y方向の大きさ)。当たり判定の半径30ユニット=0.6、直径1.2
        $wDark = 1.7; $wBody = 1.0; $wGlow = 1.8; $wCore = 0.5; $headSize = 3.5
        $eqs = @(
            (Equation 'NetLaserScale' '@O.x = @In0 * @In1
@O.y = @P.y
@O.z = @P.z'),
            (Equation 'NetLaserCenter' '@O.x = @In0 * @In1 * 0.5
@O.y = @P.y
@O.z = @P.z'),
            (Equation 'NetLaserTip' '@O.x = @In0 * @In1
@O.y = @P.y
@O.z = @P.z')
        )
        $nodes = @(
            (Node 'Dark' 'NB_Flat.png' 'Blend' @(30, 32, 52, 230) 'Fixed' @(0.5, 0, 0) 1 @(1, $wDark, 1) 0),
            (Node 'Body' 'NB_Flat.png' 'Blend' @(185, 185, 225, 255) 'Fixed' @(0.5, 0, 0) 1 @(1, $wBody, 1) 0),
            (Node 'Glow' 'NB_Soft.png' 'Add' @(255, 255, 255, 90) 'Fixed' @(0.5, 0, 0) 1 @(1, $wGlow, 1) 0),
            (Node 'Core' 'NB_Core.png' 'Add' @(255, 255, 255, 200) 'Fixed' @(0.5, 0, 0) 1 @(1, $wCore, 1) 0),
            # 横から見たときに線が消えないよう、90度ずらした板(光と芯だけ)を足す
            (Node 'GlowCross' 'NB_Soft.png' 'Add' @(255, 255, 255, 60) 'Fixed' @(0.5, 0, 0) 1 @(1, $wGlow, 1) 0 @(90, 0, 0)),
            (Node 'CoreCross' 'NB_Core.png' 'Add' @(255, 255, 255, 140) 'Fixed' @(0.5, 0, 0) 1 @(1, $wCore, 1) 0 @(90, 0, 0)),
            (Node 'Head' 'NB_Head.png' 'Add' @(255, 255, 255, 230) 'Billboard' @(0, 0, 0) 2 @($headSize, $headSize, 1) -1)
        )
        $text = Project $nodes $eqs @($testRate, $testLength, 0, 0) 60
    }
    'Warning' {
        # 当たり判定がない予告線。細く、暗め。全体が最初から出ている(伸びない)
        $wDark = 0.9; $wLight = 0.3
        $eqs = @(
            (Equation 'NetWarnScale' '@O.x = @In0
@O.y = @P.y
@O.z = @P.z'),
            (Equation 'NetWarnCenter' '@O.x = @In0 * 0.5
@O.y = @P.y
@O.z = @P.z')
        )
        $nodes = @(
            (Node 'Dark' 'NB_Flat.png' 'Blend' @(25, 28, 45, 170) 'Fixed' @(0.5, 0, 0) 1 @(1, $wDark, 1) 0),
            (Node 'Light' 'NB_Core.png' 'Add' @(215, 215, 245, 120) 'Fixed' @(0.5, 0, 0) 1 @(1, $wLight, 1) 0),
            (Node 'LightCross' 'NB_Core.png' 'Add' @(215, 215, 245, 80) 'Fixed' @(0.5, 0, 0) 1 @(1, $wLight, 1) 0 @(90, 0, 0))
        )
        $text = Project $nodes $eqs @($testLength, 0, 0, 0) 60
    }
    'Muzzle' {
        # 発射口の小さな発光。ループ。コードが止める
        $eqs = @()
        $nodes = @(
            (Node 'Flare' 'NB_Head.png' 'Add' @(255, 255, 255, 235) 'Billboard' @(0, 0, 0) -1 @(3.2, 3.2, 1) -1),
            (Node 'FlareDiag' 'NB_Head.png' 'Add' @(255, 255, 255, 150) 'Billboard' @(0, 0, 0) -1 @(2.2, 2.2, 1) -1 @(0, 0, 45))
        )
        $text = Project $nodes $eqs @(0, 0, 0, 0) 60
    }
}

$path = Join-Path $outDir "Net$($Effect)_$Suffix.efkproj"
[IO.File]::WriteAllText($path, $text, (New-Object Text.UTF8Encoding($false)))
Write-Host "wrote $path"
