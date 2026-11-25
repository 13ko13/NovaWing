# 網のエフェクトの編集用ファイル(.efkproj)を作る。数値はここで変える。
# 実行: powershell -NoProfile -ExecutionPolicy Bypass -File make_efkproj.ps1 -Effect Laser|Warning|Streak
# 出力: ../<Name>_gen.efkproj (Tools/EffekseerEq の convert で .efkefc/.efk にしたら消す)
#
# 約束: 線の長手方向=ローカル+X、原点=始点。
# 動的パラメーター(長さはゲーム単位のまま渡す。式の中で×0.02(=÷50、書き出し倍率)してエディタ単位にする):
#   Laser  : @In0 = 伸びた割合(0〜1), @In1 = 線の長さ(ゲーム単位)
#   Warning: @In0 = 線の長さ(ゲーム単位)
#   Streak : @In0 = 筋の長さ(ゲーム単位)。発射口から本番の線の先端へ伸ばす細い光(向きはゲーム側で回す)
# 動的パラメーターは「生成した瞬間」にしか計算されないので、長さが変わる線は寿命1Fで毎フレーム出し直す。
#
# v6(11/25 軽量化): 見た目はほぼそのままで、ノードを 本番29→4、予告3→2、筋4→1 にした。
#   - 重なっていた層を1枚のテクスチャに焼き込んだ(gen_net_tex.ps1 の NB_LineBake / NB_Arc3 / NB_StreakBake)
#   - 線は「Y軸固定ビルボード」(Billboard=1)で描く。板の縦(ローカルY)を Z回転-90 で+X(線の向き)に向け、
#     線を軸にしてカメラの方へ回るので、横から見ても消えない。90度ずらした板(Cross)は要らなくなった。
#     そのため長さは Scale の Y に入れる(式 *ScaleY)。テクスチャも 横=断面、縦=長手方向。
#   - スパーク20か所 → 1つ。位置の最小/最大に別の式(線の2.5%〜97.5%)を入れると、その間の乱数になる
#     (エディタ1.80.7で確認。v5までのメモ「最小/最大に別の式は同じ値になる」は誤りだった)。
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
# 1枚の板。locEq/scaleEq = 動的パラメーターの式番号(なければ-1)
# billboard: Billboard(常にカメラ向き)/ YAxis(ローカルYを軸にカメラの方へ回る。線はこれ)/ Fixed(回らない)
# colMax を渡すと色が col〜colMax の乱数になる(出し直すたびに明るさが揺れる)。
# sheets を渡すと UV をフリップブック(横に sheets コマ、1コマ sheetW×sheetH)にして、コマを乱数で選ぶ。
function Node([string]$name, [string]$tex, [string]$blend, [int[]]$col, [string]$billboard,
              [double[]]$loc, [int]$locEq, [double[]]$scale, [int]$scaleEq, [double[]]$rot = @(0, 0, 0),
              [int]$life = 1, [int]$interval = 1,
              [int[]]$colMax = $null, [int]$sheets = 0, [int]$sheetW = 128, [int]$sheetH = 8) {
    $blendNo = @{ Blend = 1; Add = 2 }[$blend]
    $bbNo = @{ Billboard = 0; YAxis = 1; Fixed = 2 }[$billboard]
    $infinite = '<MaxGeneration><Infinite>True</Infinite></MaxGeneration>'
    $gen = ''
    if ($interval -ne 1) { $gen = "<Generation><GenerationTime>$(Rnd $interval)</GenerationTime></Generation>" }
    $uv = ''
    if ($sheets -gt 0) {
        $uv = "<UV>2</UV><UVAnimation><AnimationParams><Size><X>$sheetW</X><Y>$sheetH</Y></Size><FrameLength><Infinite>True</Infinite></FrameLength><FrameCountX>$sheets</FrameCountX><FrameCountY>1</FrameCountY><StartSheet><Center>$([int](($sheets - 1) / 2))</Center><Max>$($sheets - 1)</Max><Min>0</Min></StartSheet></AnimationParams></UVAnimation>"
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
# 線から飛び出す稲妻(NB_Spark2、4x2コマ)。線上のランダムな位置に、ランダムな向き・大きさで出す。
# 位置Xの最小を式 minEq、最大を式 maxEq にすると、出すたびにその間の乱数の位置になる。
# gen = 出す間隔(最小〜最大フレーム)。1より小さくすると1フレームに何個も出る。
function SparkNode([string]$name, [double[]]$gen, [int]$minEq, [int]$maxEq, [double[]]$sx, [double[]]$sy,
                   [int[]]$col, [int[]]$colMax) {
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
function Equation([string]$name, [string]$code) {
    "<DynamicEquation><Name>$name</Name><Code>$code</Code></DynamicEquation>"
}
# 位置Xだけ / 大きさYだけを式で決める(他は @P のまま)
function EqX([string]$name, [string]$expr) { Equation $name "@O.x = $expr
@O.y = @P.y
@O.z = @P.z" }
function EqY([string]$name, [string]$expr) { Equation $name "@O.x = @P.x
@O.y = $expr
@O.z = @P.z" }
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
<ToolVersion>1.80.7</ToolVersion><Version>3</Version>
<StartFrame>0</StartFrame><EndFrame>$endFrame</EndFrame><IsLoop>True</IsLoop>
</EffekseerProject>
"@
}

# ---- 数値(エディタ単位。1=ゲーム50ユニット) ----
$testLength = $TestLength
$testRate = $TestRate
# 線の板: Z回転-90 で板の縦(ローカルY)を+Xへ向ける
$lineRot = @(0, 0, -90)

switch ($Effect) {
    'Laser' {
        # 太さ。当たり判定の半径30ユニット=0.6、直径1.2
        # v4(11/25、ユーザー「まだ少しまぶしい、ちょっと太すぎる」): 本体(当たり判定と同じくらい)はそのまま、外側の層・電気・先端を細く
        # v6: 暗い縁1.4・本体1.0・光1.4・芯0.4 の4層は NB_LineBake に焼き込んだ(層の太さ・色は gen_net_tex.ps1)。板の太さは一番外の1.4
        $wLine = 1.4; $wArc = 3.5; $headSize = 3.0
        $len = '@In0 * @In1 * 0.02'   # 伸びた分の長さ(エディタ単位)
        $eqs = @(
            (EqY 'NetLaserScaleY' $len),
            (EqX 'NetLaserCenter' '@In0 * @In1 * 0.01'),
            (EqX 'NetLaserTip' $len),
            # スパークの位置の最小/最大(線の2.5%〜97.5%。伸びている途中は伸びた分の中)
            (EqX 'NetSparkMin' "0.025 * $len"),
            (EqX 'NetSparkMax' "0.975 * $len")
        )
        # 色は緑。外側の暗い層も黒ではなく深い緑(曇り空で埋もれない程度の濃さは残す)
        # Line = 線の4層を焼いた1枚(Blend)。光の明るさの揺れは、明るさ違い8コマを乱数で選んで出す
        # Arc  = 線に沿うビリビリ(NB_Arc3 = 以前の Arc と Arc2 を重ねて焼いたもの。8コマを乱数、毎フレーム出し直す)
        #        色の乱数は以前の Arc の範囲(90,235,130,65〜150,255,180,120)を、最大を255とした比で掛ける
        # Spark = 線から飛び出す稲妻(1フレームに約3個、寿命2〜4F)
        $nodes = @(
            (Node 'Line' 'NB_LineBake.png' 'Blend' @(255, 255, 255, 255) 'YAxis' @(0.5, 0, 0) 1 @($wLine, 1, 1) 0 $lineRot -sheets 8 -sheetW 128 -sheetH 8),
            (Node 'Arc' 'NB_Arc3.png' 'Add' @(153, 235, 184, 138) 'YAxis' @(0.5, 0, 0) 1 @($wArc, 1, 1) 0 $lineRot -colMax @(255, 255, 255, 255) -sheets 8 -sheetW 64 -sheetH 1024),
            (Node 'Head' 'NB_Head.png' 'Add' @(200, 255, 215, 175) 'Billboard' @(0, 0, 0) 2 @($headSize, $headSize, 1) -1)
        )
        # スパーク: 以前は20か所×3〜10Fごと(全体で1フレームに約3個)。今は1つで0.2〜0.45Fごと(平均3個/F)。v4で少し小さく・暗く
        $nodes += SparkNode 'Spark' @(0.2, 0.45) 3 4 @(3, 6) @(1.5, 3) @(100, 240, 140, 80) @(160, 255, 190, 135)
        $text = Project $nodes $eqs @($testRate, $testLength, 0, 0) 60
    }
    'Warning' {
        # 当たり判定がない予告線。細く、暗め。全体が最初から出ている(伸びない)
        # v4(11/25、ユーザー「予告の主張が強くて本番に見える」): 細く薄くして、電気とスパークは本番だけにした
        # (「ビリビリしたら当たる」で見分ける)。太さは本番の芯(0.5)より細い。
        $wDark = 0.45; $wLight = 0.15
        $eqs = @(
            (EqX 'NetWarnScale' '@In0 * 0.02'),
            (EqX 'NetWarnCenter' '@In0 * 0.01')
        )
        # 緑。電気(Arc)とスパークはなし
        # v5(11/25、ユーザー「ちょっと点滅させれない？」): 暗い層は出しっぱなし、明るい芯だけ点滅。
        # 寿命 $blinkOn F で $blinkCycle F ごとに出す(= $blinkOn F 光る → 残りは消える)。
        # 予告の長さは再生したときに1回渡すだけで変わらないので、毎フレーム出し直さなくてよい。
        # v6: 90度ずらした芯(LightCross)は、カメラが網をほぼ正面から見るので外した(予告は正面から見えればよい)
        #     暗い層は寿命1Fのまま毎フレーム出し直す。寿命1Fを毎フレーム出すと2個重なって描かれるので(エディタで確認)、
        #     寿命を延ばすと薄くなる。濃さを変えないためにそのままにした(1個/フレームなので軽い)
        $blinkOn = 6; $blinkCycle = 12
        $nodes = @(
            (Node 'Dark' 'NB_Flat.png' 'Blend' @(12, 55, 30, 70) 'Fixed' @(0.5, 0, 0) 1 @(1, $wDark, 1) 0),
            (Node 'Light' 'NB_Core.png' 'Add' @(130, 255, 160, 110) 'Fixed' @(0.5, 0, 0) 1 @(1, $wLight, 1) 0 -life $blinkOn -interval $blinkCycle)
        )
        $text = Project $nodes $eqs @($testLength, 0, 0, 0) 60
    }
    'Streak' {
        # 発射口→本番の線の先端の細い光の筋。本番の線(太さ1.0)より細く、暗い縁取りなし。
        # v6: 光(太さ0.7、明るさが乱数)+芯(0.18)を NB_StreakBake に焼き込み、Y軸固定ビルボード1枚にした
        #     (以前は板2枚×90度ずらしの4枚)。筋は奥行き方向に伸びるが、線を軸にカメラへ回るので消えない。
        $wStreak = 0.7
        $eqs = @(
            (EqY 'NetStreakScaleY' '@In0 * 0.02'),
            (EqX 'NetStreakCenter' '@In0 * 0.01')
        )
        $nodes = @(
            (Node 'Streak' 'NB_StreakBake.png' 'Add' @(255, 255, 255, 255) 'YAxis' @(0.5, 0, 0) 1 @($wStreak, 1, 1) 0 $lineRot -sheets 8 -sheetW 128 -sheetH 8)
        )
        $text = Project $nodes $eqs @(3000, 0, 0, 0) 60
    }
}

$path = Join-Path $outDir "Net$($Effect)_$Suffix.efkproj"
[IO.File]::WriteAllText($path, $text, (New-Object Text.UTF8Encoding($false)))
Write-Host "wrote $path"
