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

static const float wave_frequency = 0.005f; //波の周波数(小さいほど波が長い)
static const float wave_speed = 1.0f; //波の速度
static const float wave_height = 50.0f; //波の高さ

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
    time * wave_speed) * wave_height;
    
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
    float derivateveWave =
    cos(
    worldPos.x * wave_frequency +
    time * wave_speed) * wave_height + wave_frequency;
    
    //傾きから法線を計算する
    float3 newNormal = normalize(float3(-derivateveWave, 1.0f, 0.0));
    
    output.pos = screenPos;
    output.uv = input.uv;
    output.normal = worldNormal;
    output.worldPos = worldPos.xyz;
    
    return output;
}