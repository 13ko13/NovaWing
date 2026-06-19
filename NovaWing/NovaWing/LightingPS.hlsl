Texture2D tex : register(t0);
SamplerState smp : register(s0);

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
};

cbuffer LightingBuffer : register(b0)
{
    float3 lightVec;//光の方向ベクトル
};

float4 main(PS_INPUT input) : SV_TARGET
{
    return float4(normalize(lightVec), 1.0f); // lightVecの値を直接確認
    
    //ピクセル情報を取得
    float4 texColor = tex.Sample(smp, input.uv);

    //法線を正規化
    float3 normVec = normalize(input.normal);
    //光のベクトルを正規化
    // 一時的に固定値で確認
    float3 normLightVec = normalize(lightVec);
    // アンビエントを追加（完全な暗闇を防ぐ）
    float ambient = 0.3f;
    
    //その二つの内積を計算
    //内積の結果が1:明るい,0以下:暗い
    //内積は-1も出るが、0以下は光があたっていないのでsaturteで0～1にクランプ
    float diffuse = saturate(dot(normVec, normLightVec));
    
    float light = saturate(diffuse + ambient);

    //テクスチャの色に明るさを適用してそのピクセルの色を返す
    return float4(texColor.rgb * light, 1.0f);
}