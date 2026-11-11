Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

struct PS_INPUT
{
	float4 pos : SV_Position;
	float4 dif : COLOR0;
	float4 spc : COLOR1;
	float2 uv : TEXCOORD0;
};

// 仮で水滴の中心を用意しておく
static const float2 water_drop_center = float2(0.7f, 0.4f);
// 水滴がどのくらいの大きさか
static const float water_drop_radius = 0.03f;
// 水滴の中の景色をどのぐらい縮めるか
static const float shrink_ratio = 2.0f;
// 仮で画面の横幅と縦幅
static const float window_width = 1280.0f;
static const float window_height = 720.0f;
// 比率を合わせる必要があるので画面のアスペクト比を計算
static const float screen_aspect_ratio = window_width / window_height;
//水滴を指定の方向にどれだけ大きくするか
static const float water_drop_stretch = 0.7f;

float4 main(PS_INPUT input) : SV_TARGET
{
	// 元のuv
	float2 uv = input.uv;

	// ピクセルが水滴の中心からのベクトルを出す
	float2 waterDropToUV = uv - water_drop_center;
	// 水滴の半径を縦も横もそろえる必要があるので
	// ずらした後のuv.yを倍率で割ることで1にする
	float2 correctedWaterDropToUV = float2(
		uv.x - water_drop_center.x,
		(uv.y - water_drop_center.y) / screen_aspect_ratio * water_drop_stretch);

	// 水滴の中心からの距離を出す
	float waterDropToUVLength = length(correctedWaterDropToUV);

	// 水滴の中心からピクセルまでの距離が0なら1
	// 距離が半径なら0になる式を立てる
	float waterToFrameRatio = saturate(1.0f - waterDropToUVLength / water_drop_radius);

	// 水滴の中だけ読むuvをずらす
	float2 distortionUV;

	// 水滴の中心 + ずらす方向 * 縮める割合
	// 中心からuvまでのベクトルはすでに持っているのでそれを反転させることでずらす方向にする
	distortionUV = water_drop_center + (-waterDropToUV) * shrink_ratio;

	// ずらしたuvを使って色を水滴の中の色を持っておく
	float4 colInWater = tex.Sample(smp, distortionUV);
	// 水滴の外の色も持っておく
	float4 colOutWater = tex.Sample(smp, uv);
	// その二つを縁に近づくにつれて色が混ざるようにする
	// waterToFrameRatioは水滴の外に行くにつれて0になる
	return lerp(colOutWater, colInWater, waterToFrameRatio);
}
