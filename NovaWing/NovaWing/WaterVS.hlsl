struct VS_INPUT
{
    float4 pos : POSITION; //モデルの頂点座標
    float3 normal : NORMAL0; //モデルの法線方向
    float4 diffuse : COLOR0; //この二つはないとうまく値が入らない
    float4 specular : COLOR1;
    float2 uv : TEXCOORD0; //ピクセルuv
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION; //スクリーン空間の位置
    float2 uv : TEXCOORD0; //UV座標
    float3 normal : NORMAL; //ワールド空間の法線
    float3 worldPos : TEXCOORD1; //ピクセルシェーダーに渡すワールド座標
};

cbuffer MatrixBuffer : register(b2)
{
    float4x4 world; //ワールド行列
    float4x4 view; //カメラ行列
    //射影行列(空間座標を視錐台に収め、画面上の2次元座標へ変換するための行列)
    //遠近法などを表すためには必須
    float4x4 proj; //プロジェクション行列
}

cbuffer WaterBuffer : register(b4)
{
    float time; //経過時間
    float3 padding; //詰め物(16バイトアライメント)
}

//ベクトルのXとZからランダム倍に見える値を作る関数
float Hash(float2 vecXZ)
{
    //fracは小数点以下を切り出してくれる
    float value = dot(vecXZ, float2(12.9898, 78.233));
    return frac(sin(value) * 43758.5453);
}

float ValueNoise(float2 vecXZ)
{
    //頂点の位置から、自分がいるグリッド内の場所を求める
    float2 gridCell = floor(vecXZ); //マス目の左下の座標
    //fracは小数点以下のみ切り出す
    float2 localPos = frac(vecXZ); //マス目内での位置(0～1)
    //そのマス目の四隅をランダムな値にしてくれるハッシュ値を求める
    float hash00 = Hash(gridCell + float2(0.0f, 0.0f)); //左下
    float hash10 = Hash(gridCell + float2(1.0f, 0.0f)); //右下
    float hash01 = Hash(gridCell + float2(0.0f, 1.0f)); //左上
    float hash11 = Hash(gridCell + float2(1.0f, 1.0f)); //右上
    //下辺をlocalPosのxで補間する
    float bottomValue = lerp(hash00, hash10, localPos.x);
    //上辺をlocalPosのxで補間する
    float topValue = lerp(hash01, hash11, localPos.x);
    //最後にこの二つをlocalPosのyで補間する
    float finalValue = lerp(bottomValue, topValue, localPos.y);
    
    return finalValue;
}

//横の波
static const float wave_frequency = 0.0025f; //波の周波数(小さいほど波が長い)
static const float wave_speed = 1.0f; //波の速度
static const float wave_height = 33.0f; //波の高さ

//縦の波
static const float wave_frequency2 = 0.005f; //波の周波数(小さいほど波が長い)
static const float wave_speed2 = 0.1f; //波の速度
static const float wave_height2 = 10.0f; //波の高さ

//斜めの小さい波
static const float wave_frequency3 = 0.02f; //波の周波数(小さいほど波が長い)
static const float wave_speed3 = 2.0f; //波の速度
static const float wave_height3 = 4.0f; //波の高さ

//細かい波紋
static const float wave_frequency4 = 0.05f; //波の周波数(小さいほど波が長い)
static const float wave_speed4 = 2.5f; //波の速度
static const float wave_height4 = 0.5f; //波の高さ

//白波を出すための波
static const float wave_frequency5 = 0.001f; //波の周波数(小さいほど波が長い)
static const float wave_speed5 = 3.0f; //波の速度
static const float wave_height5 = 40.0f; //波の高さ

//ノイズの細かさを調整するための値
static const float noiseScale = 0.06f;
//傾きを求めるためにずらす量(微分で使用する)
static const float noiseEpsilon = 0.0001f;
//ノイズの強さ
static const float noiseStrength = 0.05f;

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    
    //ローカル座標はモデル自身を中心としたときの座標
    //ワールド座標はモデル自身が世界の中心から見てどこにあるかの座標
    //ビュー座標はカメラの位置を基準としたときの座標
    
	//ローカル座標→ワールド座標に変換する
    float4 worldPos = mul(input.pos, transpose(world));
    
    //頂点のワールド座標を計算した後、波の変位を加える
    //sin(頂点のワールド座標X * 波の周波数 + 経過時間 * 波の速度 ) * 波の高さ
    float wave = sin(worldPos.x * wave_frequency +
    time * wave_speed) * wave_height + //横の波
    sin(worldPos.z * wave_frequency2 +
    time * wave_speed2) * wave_height2 + //縦の波
    sin((worldPos.z + worldPos.x) * wave_frequency3 +
    time * wave_speed3) * wave_height3 + //斜めの波
    sin((worldPos.z - worldPos.x) * wave_frequency4 +
    time * wave_speed4) * wave_height4 + //細かい波紋
    sin((worldPos.z + worldPos.x) * wave_frequency5 +
    time * wave_speed5) * wave_height5;
    
    //ワールド座標のYに波の変位を加える必要がある
    worldPos.y += wave;
    
    //ワールド座標→ビュー座標
    float4 viewPos = mul(worldPos, transpose(view));
    //ビュー座標をスクリーン座標に変換
    float4 screenPos = mul(viewPos, transpose(proj));
    //法線のワールド変換
    //wの値を0にすることで、方向として変換、xyzのみをとることでfloat3に戻す
    float3 worldNormal = normalize(mul(float4(input.normal.xyz, 0.0f), transpose(world)).xyz);
    
    //波の微分から法線を計算
    //波の傾きを計算する
    //X方向の傾き
    float derivateveWaveX =
    cos(
             worldPos.x * wave_frequency +
              time * wave_speed
          ) * wave_height * wave_frequency;
    
    //Z方向の傾き
    float derivateveWaveZ =
    cos(
             worldPos.z * wave_frequency2 +
              time * wave_speed2
          ) * wave_height2 * wave_frequency2;
    
    //斜めの波Xの傾き(Diagonal=斜め)
    float dDiagonalWaveX =
    cos(
             (worldPos.z + worldPos.x) * wave_frequency3 +
              time * wave_speed3
          ) * wave_height3 * wave_frequency3;
    
    //斜めの波Zの傾き(Diagonal=斜め)
    //斜め45度なのでXとZの傾きは同じ
    float dDiagonalWaveZ = dDiagonalWaveX;
    
    //細かい波紋Xの傾き(Ripple=波紋)
    float dRippleWaveX =
    cos(
             (worldPos.z - worldPos.x) * wave_frequency4 +
              time * wave_speed4
          ) * wave_height4 * wave_frequency4;
    
    //細かい波紋Zの傾き(Ripple=波紋)
    //X-ZなのでZはXと逆向き
    float dRippleWaveZ = -dRippleWaveX;
    
    //白波用の波X
    float dWhiteWaveX =
    cos(
             (worldPos.z + worldPos.x) * wave_frequency5 +
              time * wave_speed5
          ) * wave_height5 * wave_frequency5;
    
    //白波用の波Z
    float dWhiteWaveZ = dWhiteWaveX;
    
    //波が滑らかすぎて布のように見えるのでノイズを少しかけてみる
    //基準点のノイズ
    float noiseBase = ValueNoise(worldPos.xz * noiseScale);
    //x方向に少しずらした点のノイズ
    float noiseX = ValueNoise(worldPos.xz *
    noiseScale + float2(noiseEpsilon, 0.0f));
    //z方向に少しずらした点のノイズ
    float noiseZ = ValueNoise(worldPos.xz *
    noiseScale + float2(0.0f, noiseEpsilon));
    
    //傾きから法線を計算する
    //↓波が右上がりの場合
    //波が右上がりの坂状態ということは
    //その法線は左上を向くので-を付ける必要がある
    //全ての傾きを足し合わせる
    //この二つから傾きを求める
    float noiseDX = (noiseX - noiseBase) / noiseEpsilon;
    float noiseDZ = (noiseZ - noiseBase) / noiseEpsilon;
    
    //最終的な法線を計算
    float3 finalNormal = normalize(float3(
    -(derivateveWaveX + dDiagonalWaveX +
    dRippleWaveX + dWhiteWaveX + noiseDX * noiseStrength), //X成分を持つ波
    1.0f,
    -(derivateveWaveZ + dDiagonalWaveZ +
    dRippleWaveZ + dWhiteWaveZ + noiseDZ * noiseStrength))); //Z成分を持つ波
    
    output.pos = screenPos;
    output.uv = input.uv;
    output.normal = finalNormal;
    output.worldPos = worldPos.xyz;
    
    return output;
}