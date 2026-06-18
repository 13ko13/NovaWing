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
    //ピクセル情報を取得
    float4 texColor = tex.Sample(smp, input.uv);

    //法線を正規化
    float3 normVec = normalize(input.normal);
    //光のベクトルを正規化
    float3 normLightVec = normalize(lightVec);
    
    //その二つの内積を計算
    //内積の結果が1:明るい,0以下:暗い
    //内積は-1も出るが、0以下は光があたっていないのでsaturteで0～1にクランプ
    float diffuse = saturate(dot(normVec, normLightVec));

    //テクスチャの色に明るさを適用してそのピクセルの色を返す
    return float4(texColor.rgb * diffuse, 1.0f);
}