# 網のエフェクトの編集用ファイル(.efkproj)を作る。数値はここで変える。
# 実行: powershell -NoProfile -ExecutionPolicy Bypass -File make_efkproj.ps1 -Effect Laser|Warning|Streak
# 出力: ../<Name>_gen.efkproj (エディタで開いて .efkefc に保存したら消す)
#
# 約束: 線の長手方向=ローカル+X、原点=始点。
# 動的パラメーター(長さはゲーム単位のまま渡す。式の中で×0.02(=÷50、書き出し倍率)してエディタ単位にする):
#   Laser  : @In0 = 伸びた割合(0〜1), @In1 = 線の長さ(ゲーム単位)
#   Warning: @In0 = 線の長さ(ゲーム単位)
#   Streak : @In0 = 筋の長さ(ゲーム単位)。発射口から本番の線の先端へ伸ばす細い光(向きはゲーム側で回す)
# 動的パラメーターは「生成した瞬間」にしか計算されないので、線は寿命1Fで毎フレーム出し直す。
param([Parameter(Mandatory = $true)][ValidateSet('Laser', 'Warning', 'Streak')][string]$Effect,
      [double]$TestLength = 7000.0,  # 確認用の線の長さ(ゲーム単位)。実物は約7071
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
# colMax を渡すと色が col〜colMax の乱数になる(出し直すたびに明るさが揺れる)。
# sheets を渡すと UV をフリップブック(縦に sheets コマ、1コマ sheetW×sheetH)にして、コマを乱数で選ぶ。
function Node([string]$name, [string]$tex, [string]$blend, [int[]]$col, [string]$billboard,
              [double[]]$loc, [int]$locEq, [double[]]$scale, [int]$scaleEq, [double[]]$rot = @(0, 0, 0),
              [int]$life = 1, [int]$interval = 1, [double[]]$velocity = @(0, 0, 0),
              [int[]]$colMax = $null, [int]$sheets = 0, [int]$sheetW = 1024, [int]$sheetH = 64) {
    $blendNo = @{ Blend = 1; Add = 2 }[$blend]
    $bbNo = @{ Billboard = 0; Fixed = 2 }[$billboard]
    $infinite = '<MaxGeneration><Infinite>True</Infinite></MaxGeneration>'
    $gen = ''
    if ($interval -ne 1) { $gen = "<Generation><GenerationTime>$(Rnd $interval)</GenerationTime></Generation>" }
    $uv = ''
    if ($sheets -gt 0) {
        $uv = "<UV>2</UV><UVAnimation><AnimationParams><Size><X>$sheetW</X><Y>$sheetH</Y></Size><FrameLength><Infinite>True</Infinite></FrameLength><FrameCountY>$sheets</FrameCountY><StartSheet><Center>$([int](($sheets - 1) / 2))</Center><Max>$($sheets - 1)</Max><Min>0</Min></StartSheet></AnimationParams></UVAnimation>"
    }
    $color = "<Fixed><R>$($col[0])</R><G>$($col[1])</G><B>$($col[2])</B><A>$($col[3])</A></Fixed>"
    if ($colMax) {
        $ch = 'R', 'G', 'B', 'A'
        $rand = (0..3 | ForEach-Object { "<$($ch[$_])><Max>$($colMax[$_])</Max><Min>$($col[$_])</Min></$($ch[$_])>" }) -join ''
        $color = "<Type>1</Type>$color<Random>$rand</Random>"
    }
    @"
<Node>
<CommonValues>$infinite<Life>$(Rnd $life)</Life>$gen</CommonValues>
<LocationValues><Type>1</Type><PVA><Location>$(Vec3 $loc[0] $loc[1] $loc[2] $locEq)</Location></PVA></LocationValues>
<RotationValues><Type>0</Type><Fixed><Rotation><X>$($rot[0])</X><Y>$($rot[1])</Y><Z>$($rot[2])</Z></Rotation></Fixed></RotationValues>
<ScalingValues><Type>1</Type><PVA><Scale>$(Vec3 $scale[0] $scale[1] $scale[2] $scaleEq)</Scale></PVA></ScalingValues>
<RendererCommonValues><ColorTexture>Texture/$tex</ColorTexture><Wrap>1</Wrap><AlphaBlend>$blendNo</AlphaBlend>$uv</RendererCommonValues>
<DrawingValues><ColorAll>$color</ColorAll><Sprite><Billboard>$bbNo</Billboard></Sprite></DrawingValues>
<Name>$name</Name>
<Children></Children>
</Node>
"@
}
# 線から飛び出す稲妻(NB_Spark、4x2コマ)。線上のランダムな位置に、ランダムな向き・大きさで出す。
# 動的パラメーターの式は乱数を作れない(rand()・@P の乱数・最小/最大に別の式、どれも毎回同じ値になった)。
# そこで線上の決まった割合の位置(式 eq)に発生点を置き、出す間隔を乱数(gen = 最小〜最大フレーム)にしてバラけさせる。
function SparkNode([string]$name, [double[]]$gen, [int]$eq, [double[]]$sx, [double[]]$sy,
                   [int[]]$col, [int[]]$colMax) {
    $minEq = $eq; $maxEq = $eq
    $ch = 'R', 'G', 'B', 'A'
    $rand = (0..3 | ForEach-Object { "<$($ch[$_])><Max>$($colMax[$_])</Max><Min>$($col[$_])</Min></$($ch[$_])>" }) -join ''
    @"
<Node>
<CommonValues><MaxGeneration><Infinite>True</Infinite></MaxGeneration><Life><Center>3</Center><Max>4</Max><Min>2</Min></Life><Generation><GenerationTime><Center>$(($gen[0] + $gen[1]) / 2)</Center><Max>$($gen[1])</Max><Min>$($gen[0])</Min></GenerationTime></Generation></CommonValues>
<LocationValues><Type>1</Type><PVA><Location><X><Center>0</Center><Max>0</Max><Min>0</Min></X><DynamicEquationMin>$minEq</DynamicEquationMin><DynamicEquationMax>$maxEq</DynamicEquationMax></Location></PVA></LocationValues>
<RotationValues><Type>1</Type><PVA><Rotation><Z><Center>0</Center><Max>180</Max><Min>-180</Min></Z></Rotation></PVA></RotationValues>
<ScalingValues><Type>1</Type><PVA><Scale><X><Center>$(($sx[0] + $sx[1]) / 2)</Center><Max>$($sx[1])</Max><Min>$($sx[0])</Min></X><Y><Center>$(($sy[0] + $sy[1]) / 2)</Center><Max>$($sy[1])</Max><Min>$($sy[0])</Min></Y></Scale></PVA></ScalingValues>
<RendererCommonValues><ColorTexture>Texture/NB_Spark2.png</ColorTexture><Wrap>1</Wrap><AlphaBlend>2</AlphaBlend><UV>2</UV><UVAnimation><AnimationParams><Size><X>128</X><Y>128</Y></Size><FrameLength><Infinite>True</Infinite></FrameLength><FrameCountX>4</FrameCountX><FrameCountY>2</FrameCountY><StartSheet><Center>3</Center><Max>7</Max><Min>0</Min></StartSheet></AnimationParams></UVAnimation></RendererCommonValues>
<DrawingValues><ColorAll><Type>1</Type><Random>$rand</Random></ColorAll><Sprite><Billboard>2</Billboard></Sprite></DrawingValues>
<Name>$name</Name>
<Children></Children>
</Node>
"@
}
# 発生点を $count か所(線の長さの (k+0.5)/count の位置)。式と節点を作って返す。
# $lenExpr = 線の長さ(エディタ単位)の式、$eqBase = 最初の式の番号
function Sparks([int]$count, [string]$lenExpr, [int]$eqBase, [double[]]$gen, [double[]]$sx, [double[]]$sy, [int[]]$col, [int[]]$colMax) {
    $eqs = @(); $nodes = @()
    for ($k = 0; $k -lt $count; $k++) {
        $f = [math]::Round(($k + 0.5) / $count, 4)
        $eqs += Equation "NetSpark$k" "@O.x = $f * $lenExpr
@O.y = @P.y
@O.z = @P.z"
        $nodes += SparkNode "Spark$k" $gen ($eqBase + $k) $sx $sy $col $colMax
    }
    @{ Eqs = $eqs; Nodes = $nodes }
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
        # v4(11/25、ユーザー「まだ少しまぶしい、ちょっと太すぎる」): 本体(当たり判定と同じくらい)はそのまま、外側の層・電気・先端を細く
        $wDark = 1.4; $wBody = 1.0; $wGlow = 1.4; $wCore = 0.4; $headSize = 3.0
        $eqs = @(
            (Equation 'NetLaserScale' '@O.x = @In0 * @In1 * 0.02
@O.y = @P.y
@O.z = @P.z'),
            (Equation 'NetLaserCenter' '@O.x = @In0 * @In1 * 0.01
@O.y = @P.y
@O.z = @P.z'),
            (Equation 'NetLaserTip' '@O.x = @In0 * @In1 * 0.02
@O.y = @P.y
@O.z = @P.z')
        )
        # 色は緑。外側の暗い層も黒ではなく深い緑にする(曇り空で埋もれない程度の濃さは残す)
        # Arc/Arc2 = 線に沿うビリビリ(NB_Arc2 のコマを乱数で選び、1〜2Fごとに出し直す)
        # Spark = 線から飛び出す稲妻(1フレームに4個、寿命2〜4F)
        $wArc = 3.5
        # v4: 光る層(Add)の明るさ(アルファ)を約3/4に
        $nodes = @(
            (Node 'Dark' 'NB_Flat.png' 'Blend' @(12, 58, 30, 170) 'Fixed' @(0.5, 0, 0) 1 @(1, $wDark, 1) 0),
            (Node 'Body' 'NB_Flat.png' 'Blend' @(60, 190, 95, 255) 'Fixed' @(0.5, 0, 0) 1 @(1, $wBody, 1) 0),
            (Node 'Glow' 'NB_Soft.png' 'Add' @(80, 255, 130, 45) 'Fixed' @(0.5, 0, 0) 1 @(1, $wGlow, 1) 0 -colMax @(140, 255, 180, 90)),
            (Node 'Core' 'NB_Core.png' 'Add' @(225, 255, 230, 155) 'Fixed' @(0.5, 0, 0) 1 @(1, $wCore, 1) 0),
            # 横から見たときに線が消えないよう、90度ずらした板(光と芯だけ)を足す
            (Node 'GlowCross' 'NB_Soft.png' 'Add' @(90, 255, 140, 45) 'Fixed' @(0.5, 0, 0) 1 @(1, $wGlow, 1) 0 @(90, 0, 0)),
            (Node 'CoreCross' 'NB_Core.png' 'Add' @(225, 255, 230, 105) 'Fixed' @(0.5, 0, 0) 1 @(1, $wCore, 1) 0 @(90, 0, 0)),
            # v3(「まぶしすぎる」): 電気とスパークの明るさを約半分にし、白から緑寄りへ
            (Node 'Arc' 'NB_Arc2.png' 'Add' @(90, 235, 130, 65) 'Fixed' @(0.5, 0, 0) 1 @(1, $wArc, 1) 0 -colMax @(150, 255, 180, 120) -sheets 8),
            (Node 'Arc2' 'NB_Arc2.png' 'Add' @(50, 200, 90, 35) 'Fixed' @(0.5, 0, 0) 1 @(1, ($wArc * 0.8), 1) 0 @(180, 0, 0) -life 2 -interval 2 -colMax @(100, 240, 140, 80) -sheets 8),
            (Node 'Head' 'NB_Head.png' 'Add' @(200, 255, 215, 175) 'Billboard' @(0, 0, 0) 2 @($headSize, $headSize, 1) -1)
        )
        # スパーク20か所、それぞれ3〜10Fごと(全体で1フレームに約3個)。伸びている途中は伸びた分の中に収まる。v4で少し小さく・暗く
        $sp = Sparks 20 '@In0 * @In1 * 0.02' $eqs.Count @(3, 10) @(3, 6) @(1.5, 3) @(100, 240, 140, 80) @(160, 255, 190, 135)
        $eqs += $sp.Eqs; $nodes += $sp.Nodes
        $text = Project $nodes $eqs @($testRate, $testLength, 0, 0) 60
    }
    'Warning' {
        # 当たり判定がない予告線。細く、暗め。全体が最初から出ている(伸びない)
        # v4(11/25、ユーザー「予告の主張が強くて本番に見える」): 細く薄くして、電気とスパークは本番だけにした
        # (「ビリビリしたら当たる」で見分ける)。太さは本番の芯(0.5)より細い。
        $wDark = 0.45; $wLight = 0.15
        $eqs = @(
            (Equation 'NetWarnScale' '@O.x = @In0 * 0.02
@O.y = @P.y
@O.z = @P.z'),
            (Equation 'NetWarnCenter' '@O.x = @In0 * 0.01
@O.y = @P.y
@O.z = @P.z')
        )
        # 緑。電気(Arc)とスパークはなし
        # v5(11/25、ユーザー「ちょっと点滅させれない？」): 暗い層は出しっぱなし、明るい芯だけ点滅。
        # 寿命 $blinkOn F で $blinkCycle F ごとに出す(= $blinkOn F 光る → 残りは消える)。予告の長さは変わらないので、出し直しが毎フレームでなくてもよい。
        $blinkOn = 6; $blinkCycle = 12
        $nodes = @(
            (Node 'Dark' 'NB_Flat.png' 'Blend' @(12, 55, 30, 70) 'Fixed' @(0.5, 0, 0) 1 @(1, $wDark, 1) 0),
            (Node 'Light' 'NB_Core.png' 'Add' @(130, 255, 160, 110) 'Fixed' @(0.5, 0, 0) 1 @(1, $wLight, 1) 0 -life $blinkOn -interval $blinkCycle),
            (Node 'LightCross' 'NB_Core.png' 'Add' @(130, 255, 160, 70) 'Fixed' @(0.5, 0, 0) 1 @(1, $wLight, 1) 0 @(90, 0, 0) -life $blinkOn -interval $blinkCycle)
        )
        $text = Project $nodes $eqs @($testLength, 0, 0, 0) 60
    }
    'Streak' {
        # 発射口→本番の線の先端の細い光の筋。本番の線(太さ1.0)より細く、暗い縁取りなし。
        # 板2枚(90度ずらし)で、どの向きから見ても消えないようにする。
        $wGlow = 0.7; $wCore = 0.18
        $eqs = @(
            (Equation 'NetStreakScale' '@O.x = @In0 * 0.02
@O.y = @P.y
@O.z = @P.z'),
            (Equation 'NetStreakCenter' '@O.x = @In0 * 0.01
@O.y = @P.y
@O.z = @P.z')
        )
        $nodes = @(
            (Node 'Glow' 'NB_Soft.png' 'Add' @(70, 255, 120, 70) 'Fixed' @(0.5, 0, 0) 1 @(1, $wGlow, 1) 0 -colMax @(120, 255, 165, 120)),
            (Node 'Core' 'NB_Core.png' 'Add' @(215, 255, 225, 210) 'Fixed' @(0.5, 0, 0) 1 @(1, $wCore, 1) 0),
            (Node 'GlowCross' 'NB_Soft.png' 'Add' @(70, 255, 120, 70) 'Fixed' @(0.5, 0, 0) 1 @(1, $wGlow, 1) 0 @(90, 0, 0) -colMax @(120, 255, 165, 120)),
            (Node 'CoreCross' 'NB_Core.png' 'Add' @(215, 255, 225, 170) 'Fixed' @(0.5, 0, 0) 1 @(1, $wCore, 1) 0 @(90, 0, 0))
        )
        $text = Project $nodes $eqs @(3000, 0, 0, 0) 60
    }
}

$path = Join-Path $outDir "Net$($Effect)_$Suffix.efkproj"
[IO.File]::WriteAllText($path, $text, (New-Object Text.UTF8Encoding($false)))
Write-Host "wrote $path"
