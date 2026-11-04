Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

struct PS_INPUT
{
	float4 Position   : SV_POSITION; // 画面上の位置
	float4 Diffuse    : COLOR0;      // 頂点の色(dif)
	float2 uv : TEXCOORD0;   // テクスチャ座標(u, v)
	float2 suv : TEXCOORD1;   // サブテクスチャ座標(su, sv)
};

float4 main(PS_INPUT input) : SV_TARGET0
{
	//uv
	float2 uv = input.uv;

	//色
	float4 col = tex.Sample(smp, uv);
	//uvを0~1ではなく、-1~1に変換
	uv = 2.0f * uv - 1.0f;
	//画面から離れるほど色が暗くなるようにする
	//dot(uv,uv)は画面中央からの距離の2乗を計算している
	//1.0fからdot(uv,uv)を引くことで、画面中央では1.0f、画面端では0.0fになる
	col *= 1.0f - dot(uv,uv);

	return col;
}
