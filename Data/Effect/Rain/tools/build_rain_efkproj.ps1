# Rain.efkproj（XML）を層の表から組み立てる（2026-11-11）
# エディタの1単位 = ゲームの50単位（書き出し倍率50、読み込み倍率1.0）。
# カメラ位置に毎フレーム SetPos、回転なし → エディタ -Z = ゲームの前、+X = 右、+Y = 上。
# 使い方: powershell -NoProfile -ExecutionPolicy Bypass -File build_rain_efkproj.ps1 -Out <Rain.efkproj>
param([string]$Out = "Rain.efkproj")
$inv = [Globalization.CultureInfo]::InvariantCulture
function Num($v){ ([double]$v).ToString($inv) }
function Rng($min,$max){ "<Center>$(Num (($min+$max)/2))</Center><Max>$(Num $max)</Max><Min>$(Num $min)</Min>" }

# 層の表: 名前, X範囲, Y範囲, Z範囲(前が負), 落下速度, 寿命, 発生間隔(F), 幅, 長さ, 色RGBA, 傾き(°)
$layers = @(
  # 近い層: 自機より手前（ゲームで150〜600前）だけ。画面の中央の列（|x|<125）には出さない。大きく少なく
  @{N="RainNearL"; X=@(-11,-2.5); Y=@(-3,12); Z=@(-12,-3);  V=@(0.75,0.9); L=@(24,28); G=1.0;  W=0.05; H=2.0; C=@(195,205,218,110); T=-20},
  @{N="RainNearR"; X=@(2.5,11);   Y=@(-3,12); Z=@(-12,-3);  V=@(0.75,0.9); L=@(24,28); G=1.0;  W=0.05; H=2.0; C=@(195,205,218,110); T=-20},
  # 中くらい: 自機より奥（ゲームで400〜1800前）。奥なので自機・敵の後ろは深度で隠れる
  @{N="RainMid";   X=@(-28,28);   Y=@(-6,22); Z=@(-36,-8);  V=@(0.75,0.9); L=@(32,36); G=0.15; W=0.07; H=1.7; C=@(190,200,214,80); T=-20},
  # 遠い層: 霞む細かい筋（ゲームで1800〜5500前）。目立たせない
  @{N="RainFar";   X=@(-70,70);   Y=@(-6,40); Z=@(-110,-36);V=@(0.75,0.9); L=@(48,52); G=0.2;  W=0.14; H=2.4; C=@(185,195,210,55); T=-20}
)

$nodes = foreach($l in $layers){
@"
      <Node>
        <CommonValues>
          <MaxGeneration>
            <Infinite>True</Infinite>
          </MaxGeneration>
          <LocationEffectType>1</LocationEffectType>
          <RotationEffectType>1</RotationEffectType>
          <ScaleEffectType>1</ScaleEffectType>
          <Life>
            $(Rng $l.L[0] $l.L[1])
          </Life>
          <Generation>
            <GenerationTime>
              $(Rng $l.G $l.G)
              <DynamicEquationMin>0</DynamicEquationMin>
              <DynamicEquationMax>0</DynamicEquationMax>
            </GenerationTime>
          </Generation>
        </CommonValues>
        <LocationValues>
          <Type>1</Type>
          <PVA>
            <Location>
              <X>$(Rng $l.X[0] $l.X[1])</X>
              <Y>$(Rng $l.Y[0] $l.Y[1])</Y>
              <Z>$(Rng $l.Z[0] $l.Z[1])</Z>
            </Location>
            <Velocity>
              <Y>$(Rng (-$l.V[1]) (-$l.V[0]))</Y>
            </Velocity>
          </PVA>
        </LocationValues>
        <RotationValues>
          <Fixed>
            <Rotation>
              <X>$(Num $l.T)</X>
            </Rotation>
          </Fixed>
        </RotationValues>
        <ScalingValues>
          <Fixed>
            <Scale>
              <X>$(Num $l.W)</X>
              <Y>$(Num $l.H)</Y>
              <Z>1</Z>
            </Scale>
          </Fixed>
        </ScalingValues>
        <RendererCommonValues>
          <ColorTexture>Texture/Rain_Streak.png</ColorTexture>
          <FadeInType>1</FadeInType>
          <FadeIn>
            <Frame>3</Frame>
          </FadeIn>
          <FadeOutType>1</FadeOutType>
          <FadeOut>
            <Frame>4</Frame>
          </FadeOut>
        </RendererCommonValues>
        <DrawingValues>
          <ColorAll>
            <Fixed>
              <R>$($l.C[0])</R>
              <G>$($l.C[1])</G>
              <B>$($l.C[2])</B>
              <A>$($l.C[3])</A>
            </Fixed>
          </ColorAll>
          <Sprite>
            <Billboard>2</Billboard>
          </Sprite>
        </DrawingValues>
        <Name>$($l.N)</Name>
        <Children>
        </Children>
      </Node>
"@
}

$xml = @"
<?xml version="1.0" encoding="utf-8"?>
<EffekseerProject>
  <Root>
    <Name>Root</Name>
    <Children>
$($nodes -join "`r`n")
    </Children>
  </Root>
  <Dynamic>
    <Inputs>
      <DynamicInput>
        <Input>0</Input>
      </DynamicInput>
      <DynamicInput>
        <Input>0</Input>
      </DynamicInput>
      <DynamicInput>
        <Input>0</Input>
      </DynamicInput>
      <DynamicInput>
        <Input>0</Input>
      </DynamicInput>
    </Inputs>
    <Equations>
      <DynamicEquation>
        <Name>RainAmount</Name>
        <Code>@O.x = @P.x * (1.0 + @In0)
@O.y = @P.y * (1.0 + @In0)
@O.z = @P.z * (1.0 + @In0)</Code>
      </DynamicEquation>
    </Equations>
  </Dynamic>
  <ProceduralModel>
    <ProceduralModels>
    </ProceduralModels>
  </ProceduralModel>
  <ToolVersion>1.80.6</ToolVersion>
  <Version>3</Version>
  <StartFrame>0</StartFrame>
  <EndFrame>240</EndFrame>
  <IsLoop>True</IsLoop>
</EffekseerProject>
"@
[IO.File]::WriteAllText($Out, $xml, (New-Object Text.UTF8Encoding $false))
"wrote $Out"
