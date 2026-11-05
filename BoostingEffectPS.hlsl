Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

struct PS_INPUT
{
	float4 pos : SV_Position;
	float4 dif : COLOR0;
	float4 spc : COLOR1;
	float2 uv : TEXCOORD0;
};

cbuffer BoostingBuffer : register(b8)
{
	float boostAmount;//ブースト量
	float3 padding;
}

// 10個のサンプリング位置のずれ（中心からの方向に、前後にずらす量）
static const float samples[10] =
{
	0.04, 0.095, 0.046, 0.021, 0.018,
	0.01, 0.02, 0.03, 0.05, 0.08
};

static const float sample_strength = 10.0f; // サンプリング強度

float4 main(PS_INPUT input) : SV_TARGET0
{
	// uv
	float2 uv = input.uv;
	// 画面中心へのベクトル
	float2 toCenter = 0.5f - uv;
	// 画面の中心までの距離
	float toCenterDist = length(toCenter);

	// 画面中心への方向
	float2 toCenterDir = normalize(toCenter);

	// 元の色
	float4 col = tex.Sample(smp, uv);

	//中心から少しずらしたピクセルの色を全部混ぜて平均の色を作る
	float4 sumCol = col;

	for (int i = 0; i < 10; i++)
	{
		sumCol += tex.Sample(smp, uv + toCenterDir * samples[i] * toCenterDist * boostAmount);
	}
	//混ぜ合わせた平均の色を作る
	sumCol /= 11.0f;

	// 中心から遠いほど強くぼかしをかける
	float strength = saturate(toCenterDist * sample_strength);

	return lerp(col, sumCol, strength);
}
