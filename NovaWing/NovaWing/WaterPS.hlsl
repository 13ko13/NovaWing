SamplerState smp : register(s0);

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normalWS : NORMAL; //ワールド空間の法線
    float3 worldPos : TEXCOORD1;
};

cbuffer LightingBuffer : register(b3)
{
    float3 lightVec;//光の方向ベクトル
}

cbuffer CameraBuffer : register(b5)
{
    float3 cameraPos;//カメラの位置
    float padding;
}

float4 main(PS_INPUT input) : SV_TARGET
{
    //return float4(0.0f, 0.3f, 0.8f, 1.0f);
    
    //ピクセル情報を取得
    float4 texCol = float4(0.0f, 0.0f, 0.8f, 1.0f);
    //視線方向を計算
    float3 viewDir = normalize(cameraPos - input.worldPos);

    //光のベクトルを計算
    float3 normLightDir = normalize(lightVec);
    
    return float4(normalize(lightVec) * 0.5 + 0.5, 1.0f);
    
    //環境光を追加(完全な暗闇を防ぐ)
    float ambient = 0.2f;
    //法線と光のベクトルの内積から暗い部分と明るい部分を出す
    float diffuse = saturate(dot(input.normalWS, -normLightDir));
    //最終的な光の強さ
    float light = saturate(diffuse + ambient);
    
    //視線方向と光の方向から反射ベクトルを計算
    float3 reflectVec = reflect(normLightDir, input.normalWS);

    //反射光の計算
    float specular = pow(saturate(dot(viewDir, reflectVec)), 10.0f);
    
    //テクスチャの色に明るさを適用してそのピクセルの色を返す
    return float4(texCol.rgb * light + specular, 1.0f);
}