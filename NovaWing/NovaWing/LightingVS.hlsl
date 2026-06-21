
struct VS_INPUT
{
    float4 pos : POSITION;//モデルの頂点座標
    float3 normal : NORMAL0;//モデルの法線方向
    float4 diffuse : COLOR0;//この二つはないとうまく値が入らない
    float4 specular : COLOR1; 
    float2 uv : TEXCOORD0;//ピクセルuv
    
    float3 tangent : TANGENT; //tangent空間のtangent(接線)
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;//スクリーン空間の位置
    float2 uv : TEXCOORD0;//UV座標
    float3 normal : NORMAL;//ワールド空間の法線
    float3 worldPos : TEXCOORD1;//ピクセルシェーダーに渡すワールド座標
    
    float3 tangent : TANGENT; //tangent空間のtangent(接線)
};

cbuffer MatrixBuffer : register(b2)
{
    float4x4 world;//ワールド行列
    float4x4 view;//カメラ行列
    //射影行列(空間座標を視錐台に収め、画面上の2次元座標へ変換するための行列)
    //遠近法などを表すためには必須
    float4x4 proj;//プロジェクション行列
}

cbuffer CameraBuffer : register(b3)
{
    float3 cameraPos; //カメラの位置
    float padding; //16バイトアライメント
}

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    
    //ローカル座標はモデル自身を中心としたときの座標
    //ワールド座標はモデル自身が世界の中心から見てどこにあるかの座標
    //ビュー座標はカメラの位置を基準としたときの座標
    
	//ローカル座標→ワールド座標に変換する
    float4 worldPos = mul(input.pos, transpose(world));
    //ワールド座標→ビュー座標
    float4 viewPos = mul(worldPos, transpose(view));
    //ビュー座標をスクリーン座標に変換
    float4 screenPos = mul(viewPos, transpose(proj));
    //法線のワールド変換
    //wの値を0にすることで、方向として変換、xyzのみをとることでfloat3に戻す
    float3 worldNormal = normalize(mul(float4(input.normal.xyz, 0.0f), transpose(world)).xyz);
    
    //tangentもワールド変換する
    float3 worldTangent = normalize(mul(float4(input.tangent.xyz, 0.0f), transpose(world)).xyz);

    output.pos = screenPos;//スクリーン空間の位置
    output.normal = worldNormal;//ワールド空間の法線
    output.tangent = worldTangent;//ワールド空間の接線
    output.uv = input.uv;//uv
    output.worldPos = worldPos.xyz;//ワールド座標
    
    return output;
}