Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

struct PS_INPUT
{
	float4 pos : SV_Position;
	float4 dif : COLOR0;
	float4 spc : COLOR1;
	float2 uv : TEXCOORD0;
};

struct WaterDropData
{
	float radius;
	float2 waterDropPos;
	float padding;
};

// 用意する水滴の数
static const int max_water_drop_count = 15;

cbuffer WaterDropBuffer : register(b9)
{
	WaterDropData waterDrops[max_water_drop_count];
}

// 水滴の中の景色をどのぐらい縮めるか
static const float shrink_ratio = 5.0f;
// 仮で画面の横幅と縦幅
static const float window_width = 1280.0f;
static const float window_height = 720.0f;
// 比率を合わせる必要があるので画面のアスペクト比を計算
static const float screen_aspect_ratio = window_width / window_height;
// 水滴を指定の方向にどれだけ大きくするか
static const float water_drop_stretch = 0.7f;
// 縁からどれぐらい内側まで色を暗くするか
static const float water_drop_rim_width = 0.005f;
// 縁をどれだけ暗くするか
static const float water_drop_rim_darkness = 0.1;
// 水滴の半径から見て光の半径をどのぐらいにするかの割合
static const float water_drop_light_radius_ratio = 0.8f;
// 水滴上の光の位置をどれだけずらすか(半径を超えたらだめなので半径を1とする)
static const float water_drop_highlight_offset = 0.05f;
// 光の強さ(1で光の中心が真っ白、0で光なし)
static const float water_drop_light_strength = 0.3f;

float4 main(PS_INPUT input) : SV_TARGET
{
	// 元のuv
	float2 uv = input.uv;
	// 初期カラー値
	float4 col = tex.Sample(smp, uv);

	for (int i = 0; i < max_water_drop_count; i++)
	{
		WaterDropData drop = waterDrops[i];
		//水滴の半径が0だったらゼロ除算になってしまうため次の水滴へ
		if (drop.radius == 0.0f)
			continue;
		// 水滴の位置から今回のピクセルまでのベクトル
		float2 waterDropToUV = uv - drop.waterDropPos;

		// 画面の横幅と高さのピクセル数は横の方が長いので
		// yのずらす量をアスペクト比で割ることでxと同じ量ずらすことができる
		float2 correctedWaterDropToUV = float2(
			uv.x - drop.waterDropPos.x,
			(uv.y - drop.waterDropPos.y) / screen_aspect_ratio);
		float correctedWaterDropToUVLength = length(correctedWaterDropToUV);

		// 水滴からピクセルまでの距離が半径を超えていたら普通の背景
		// 超えていなければ水滴内としたいので水滴の中心を1、水滴の半径を超えるにつれて0になるようにする
		float waterDropToFrameRatio = saturate(1.0f - correctedWaterDropToUVLength / drop.radius);

		// 水滴から見た上下左右が反転している景色を作る
		// 水滴の中心からピクセルまでのベクトルを反転させて水滴の中心に足す
		// 反転させるだけでは水滴っぽくないので映った景色を縮める
		float2 distortionUV = drop.waterDropPos - waterDropToUV * shrink_ratio;

		// 水滴の縁に行くほど暗くする
		// 縁に行くほど0、中心に行くほど1にする必要がある
		// 範囲が0~1だと縁が暗すぎるので0.5~1.0に変換
		float waterDropDarknessRatio = saturate(1.0f - correctedWaterDropToUVLength / drop.radius * water_drop_rim_darkness);
		// 縁を超えたら水滴部分以外の色を見せる
		// つまり0の部分は普通の景色を見せたいので
		// 水滴の中心から縁までの割合が0以下であれば暗くするための値を1にする
		if (waterDropToFrameRatio <= 0.0f)
		{
			waterDropDarknessRatio = 1.0f;
		}

		// 水滴から見てちょっと右上の当たりに光を当てる
		float2 waterLightPos = float2(
			drop.radius * water_drop_highlight_offset,
			drop.radius * 0.4f);

		// 光から今回のピクセルまでの距離を計算
		float lightToFrameLength = length(waterLightPos - correctedWaterDropToUV);

		// 光の半径
		float waterDropLightRadius = drop.radius * water_drop_light_radius_ratio;

		// 光の中心が1、光の縁が0になるような数値を作る
		float lightRatio = saturate(1.0f - lightToFrameLength / waterDropLightRadius) * water_drop_light_strength;

		// 水滴部分の色
		float4 colInWaterDrop = tex.Sample(smp, distortionUV);
		// 二つの間の縁周りだけ混ぜる
		col = lerp(col, colInWaterDrop, waterDropToFrameRatio);
		// 縁だけ暗くする
		col.rgb *= waterDropDarknessRatio;

		// 最終的に光から水滴の縁に行くにつれて水滴の色に滑らかに変わっていくようにする
		col = lerp(col, float4(1.0f, 1.0f, 1.0f, 1.0f), lightRatio);
	}

	// 完成したピクセルのカラーを返す
	return col;
}
