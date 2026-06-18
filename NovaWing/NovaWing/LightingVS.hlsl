
struct VS_INPUT
{
    float4 pos : POSITION;//モデルの頂点座標
    float3 normal : NORMAL;//モデルの法線方向
    float2 uv : TEXCOORD0;//ピクセルuv
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;//スクリーン空間の位置
    float2 uv : TEXCOORD0;//UV座標
    float3 normal : NORMAL;//ワールド空間の法線
};

cbuffer MatrixBuffer : register(b1)
{
    float4x4 world;//ワールド行列
    float4x4 view;//カメラ行列
    //射影行列(空間座標を視錐台に収め、画面上の2次元座標へ変換するための行列)
    //遠近法などを表すためには必須
    float4x4 proj;
}

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    
    //ローカル座標はモデル自身を中心としたときの座標
    //ワールド座標はモデル自身が世界の中心から見てどこにあるかの座標
    //ビュー座標はカメラの位置を基準としたときの座標
    
	//ローカル座標→ワールド座標に変換する
    float4 worldPos = mul(input.pos, world);
    //ワールド座標→ビュー座標
    float4 viewPos = mul(worldPos, view);
    //ビュー座標をスクリーン座標に変換
    float4 screenPos = mul(viewPos, proj);
    //法線のワールド変換
    //wの値を0にすることで、方向として変換、xyzのみをとることでfloat3に戻す
    float3 worldNormal = normalize(mul(float4(input.normal, 0.0f), world).xyz);

    output.pos = screenPos;//スクリーン空間の位置
    output.normal = worldNormal;//ワールド空間の法線
    output.uv = input.uv;
    
    return output;
}