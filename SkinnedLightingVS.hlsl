
//頂点シェーダに必要な情報
struct VS_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL0;
    float4 diffuse : COLOR0;
    float4 specular : COLOR1;
    float4 uv : TEXCOORD0;
    float4 suv : TEXCOORD1;
    float3 tangent : TANGENT;
    float3 bin : BINORMAL0; 
    
    int4  blendIndices : BLENDINDICES0;//影響を受けるボーン番号(最大4本)
    float4 blendWeight : BLENDWEIGHT0;//各ボーンの影響度
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION; //スクリーン空間の位置
    float2 uv : TEXCOORD0; //UV座標
    float3 normal : NORMAL; //ワールド空間の法線
    float3 worldPos : TEXCOORD1; //ピクセルシェーダーに渡すワールド座標
    
    float3 tangent : TANGENT; //tangent空間のtangent(接線)
};

cbuffer MatrixBuffer : register(b5)
{
    float4x4 world; //ワールド行列
    float4x4 view; //カメラ行列
    //射影行列(空間座標を視錐台に収め、画面上の2次元座標へ変換するための行列)
    //遠近法などを表すためには必須
    float4x4 proj; //プロジェクション行列
}

cbuffer CameraBuffer : register(b6)
{
    float3 cameraPos; //カメラの位置
    float padding; //16バイトアライメント
}

cbuffer LocalWorldMatrixBuffer : register(b3)
{
    float4 localWorldMatrix[162];//1ボーンにつきfloat4が3行、54ボーン分
}

VS_OUTPUT main(VS_INPUT input)
{ 
	//スキニング行列の合成
    float4 skinRow0 = localWorldMatrix[input.blendIndices.x + 0] * input.blendWeight.x;
    float4 skinRow1 = localWorldMatrix[input.blendIndices.x + 1] * input.blendWeight.x;
    float4 skinRow2 = localWorldMatrix[input.blendIndices.x + 2] * input.blendWeight.x;

    skinRow0 += localWorldMatrix[input.blendIndices.y + 0] * input.blendWeight.y;
    skinRow1 += localWorldMatrix[input.blendIndices.y + 1] * input.blendWeight.y;
    skinRow2 += localWorldMatrix[input.blendIndices.y + 2] * input.blendWeight.y;
    
    skinRow0 += localWorldMatrix[input.blendIndices.z + 0] * input.blendWeight.z;
    skinRow1 += localWorldMatrix[input.blendIndices.z + 1] * input.blendWeight.z;
    skinRow2 += localWorldMatrix[input.blendIndices.z + 2] * input.blendWeight.z;
    
    skinRow0 += localWorldMatrix[input.blendIndices.w + 0] * input.blendWeight.w;
    skinRow1 += localWorldMatrix[input.blendIndices.w + 1] * input.blendWeight.w;
    skinRow2 += localWorldMatrix[input.blendIndices.w + 2] * input.blendWeight.w;

    //頂点の変換を行う
    float4 worldPos;
    worldPos.x = dot(float4(input.pos.xyz, 1.0f), skinRow0);
    worldPos.y = dot(float4(input.pos.xyz, 1.0f), skinRow1);
    worldPos.z = dot(float4(input.pos.xyz, 1.0f), skinRow2);
    worldPos.w = 1.0f;
    
    VS_OUTPUT output;
    
    //ローカル座標はモデル自身を中心としたときの座標
    //ワールド座標はモデル自身が世界の中心から見てどこにあるかの座標
    //ビュー座標はカメラの位置を基準としたときの座標
	//ローカル座標→ワールド座標に変換する
    //ワールド座標→ビュー座標
    float4 viewPos = mul(worldPos, transpose(view));
    //ビュー座標をスクリーン座標に変換
    float4 screenPos = mul(viewPos, transpose(proj));
    
    //法線のワールド変換
    //wの値を0にすることで、方向として変換、xyzのみをとることでfloat3に戻す
    float4 normalVec = float4(input.normal.xyz, 0.0f);
    float3 worldNormal;
    worldNormal.x = dot(normalVec, skinRow0);
    worldNormal.y = dot(normalVec, skinRow1);
    worldNormal.z = dot(normalVec, skinRow2);
    worldNormal = normalize(worldNormal);
    
    //tangentもワールド変換する
    float4 tangetVec = float4(input.tangent.xyz, 0.0f);
    float3 worldTangent;
    worldTangent.x = dot(tangetVec, skinRow0);
    worldTangent.y = dot(tangetVec, skinRow1);
    worldTangent.z = dot(tangetVec, skinRow2);
    worldTangent = normalize(worldTangent);

    output.pos = screenPos; //スクリーン空間の位置
    output.normal = worldNormal; //ワールド空間の法線
    output.tangent = worldTangent; //ワールド空間の接線
    output.uv = input.uv.xy; //uv
    output.worldPos = worldPos.xyz; //ワールド座標
    
    return output;
}