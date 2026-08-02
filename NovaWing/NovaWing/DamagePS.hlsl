#include "LightingCommon.hlsli"

Texture2D<float4> tex : register(t0);
Texture2D<float4> normTex : register(t1);
Texture2D<float4> metalicTex : register(t2);
Texture2D<float4> emissionTex : register(t3);
SamplerState smp : register(s0);

struct PS_INPUT
{
    float4 pos : SV_Position; //スクリーン座標
    float2 uv : TEXCOORD0; //UV座標
    float3 normalWS : NORMAL; //ワールド空間の法線
    float3 worldPos : TEXCOORD1; //ワールド空間の位置
    float3 tangentWS : TANGENT; //ワールド空間の接線
};

cbuffer LightingBuffer : register(b4)
{
    float3 lightVec; //光の方向ベクトル
    float padding;//16バイトアライメント
}

cbuffer CameraBuffer : register(b5)
{
    float3 cameraPos; //カメラの位置
    float padding2; //16バイトアライメント
}

cbuffer DamageBuffer : register(b6)
{
    float redAmount; //赤の強さ
    float3 padding3; //16バイトアライメント
}

float4 main(PS_INPUT input) : SV_TARGET
{
    //法線マップの色
    float4 normMapCol = normTex.Sample(smp, input.uv);
    //メタリックマップの色
    float4 metMapCol = metalicTex.Sample(smp, input.uv);
    //ベースの色
    float4 baseCol = tex.Sample(smp, input.uv);
    //エミッションの色
    float4 emmisionCol = emissionTex.Sample(smp, input.uv);
    
    //ライティング計算を行う
    LightingResult result = CalcLighting(
        normMapCol, //法線マップの色
        metMapCol, //メタリックマップの色
        input.normalWS, //ワールド空間の法線
        input.tangentWS, //ワールド空間の接線
        lightVec, //光の方向
        cameraPos, //カメラの位置
        input.worldPos//ピクセルのワールド座標
    );
    
    //ライトを適用した色と、真赤に向かってredAmountの割合だけ混ぜる
    float4 finalCol = float4(lerp(
                        float3(baseCol.rgb * result.light + result.specular + emmisionCol.rgb),
                            float3(1.0f, 0.0f, 0.0f), redAmount), 1.0f);
    
	return finalCol;
}