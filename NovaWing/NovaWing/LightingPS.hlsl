#include "LightingCommon.hlsli"

Texture2D<float4> tex : register(t0);
Texture2D<float4> normTex : register(t1);
Texture2D<float4> metalicTex : register(t2);
Texture2D<float4> emissionTex : register(t3);
Texture2D<float4> noiseTex : register(t4);

SamplerState smp : register(s0);

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normalWS : NORMAL;//ワールド空間の法線
    float3 worldPos : TEXCOORD1;
    
    float3 tangentWS : TANGENT;//ワールド空間の接線
};

cbuffer LightingBuffer : register(b4)
{
    float3 lightVec;//光の方向ベクトル
};

cbuffer CameraBuffer : register(b6)
{
    float3 cameraPos;
    float padding;
}

//カメラ
static const float near = 200.0f;//カメラのニア

//ディゾルブ
static const float start_disolve = 400.0f;//ディゾルブ開始距離
static const float noise_uv_scale = 100.0f;//ノイズUVの縮小率

float4 main(PS_INPUT input) : SV_TARGET
{
    //物体がNearに近づくにつれてディゾルブで消えていくようにする
    //カメラからピクセルまでの距離を求める
    float cameraToPixelD = distance(cameraPos,input.worldPos);
    //この距離がNearに近づくにつれてノイズの閾値をあげていく
    float noise_threshold = 1.0f - smoothstep(near,start_disolve,cameraToPixelD);
    
    //ノイズをサンプリングする
    //岩のuvが変なので、worldPosを使ったuvを作成する
    float2 worldBaseUV = input.worldPos.xy / noise_uv_scale;
    float3 noiseCol = noiseTex.Sample(smp,worldBaseUV);

    //ノイズのカラーの値が閾値よりも小さければそのピクセルの描画を行わない
    if(noiseCol.r < noise_threshold) discard;
    
    //メタリックマップのカラーを取得
    float4 metCol = metalicTex.Sample(smp, input.uv);
    
    //ピクセル情報を取得
    float4 texColor = tex.Sample(smp, input.uv);
    //法線マップのカラーを取得(0～1)
    float4 normMapColor = normTex.Sample(smp, input.uv);
    //エミッション(影の場所でも光る)
    float4 emission = emissionTex.Sample(smp, input.uv);
    
	//ライティングの計算を行う
	LightingResult result = CalcLighting(
		normMapColor,
		metCol,
		input.normalWS,
		input.tangentWS,
		lightVec,
		cameraPos,
		input.worldPos
	);
    
    //拡散色テクスチャのアルファをスペキュラ強度マップとして使用
    //(このシェーダーは元々テクスチャのアルファを使っておらず、出力アルファも1.0f固定なので競合しない。
    //アルファチャンネルを持たないテクスチャはサンプル結果が1.0になるため既存モデルへの影響もない)
    result.specular *= texColor.a;

    //テクスチャの色に明るさを適用してそのピクセルの色を返す
    return float4(texColor.rgb * result.light + result.specular + emission.rgb, 1.0f);
}