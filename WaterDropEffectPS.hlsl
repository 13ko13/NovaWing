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
static const float shrink_ratio = 2.0f;
// 仮で画面の横幅と縦幅
static const float window_width = 1280.0f;
static const float window_height = 720.0f;
// 比率を合わせる必要があるので画面のアスペクト比を計算
static const float screen_aspect_ratio = window_width / window_height;
// 水滴を指定の方向にどれだけ大きくするか
static const float water_drop_stretch = 0.7f;
// 縁からどれぐらい内側まで色を暗くするか
static const float water_drop_rim_width = 0.1f;
// 縁をどれだけ暗くするか
static const float water_drop_rim_darkness = 0.4;
// 水滴の半径から見て光の半径をどのぐらいにするかの割合
static const float water_drop_light_radius_ratio = 0.5f;
// 水滴上の光の位置をどれだけずらすか(半径を超えたらだめなので半径を1とする)
static const float water_drop_highlight_offset = 0.4f;

float4 main(PS_INPUT input) : SV_TARGET
{
	// // 元のuv
	// float2 uv = input.uv;

	// // ピクセルが水滴の中心からのベクトルを出す
	// float2 waterDropToUV = uv - water_drop_center;
	// // 水滴の半径を縦も横もそろえる必要があるので
	// // ずらした後のuv.yを倍率で割ることで1にする
	// float2 correctedWaterDropToUV = float2(
	// 	uv.x - water_drop_center.x,
	// 	(uv.y - water_drop_center.y) / screen_aspect_ratio * water_drop_stretch);

	// // 水滴の中心からの距離を出す
	// float waterDropToUVLength = length(correctedWaterDropToUV);

	// // 水滴の中心からピクセルまでの距離が0なら1
	// // 距離が半径なら0になる式を立てる
	// float waterToFrameRatio = saturate(1.0f - waterDropToUVLength / water_drop_radius);

	// // 水滴の中だけ読むuvをずらす
	// float2 distortionUV;

	// // 水滴の中心 + ずらす方向 * 縮める割合
	// // 中心からuvまでのベクトルはすでに持っているのでそれを反転させることでずらす方向にする
	// distortionUV = water_drop_center + (-waterDropToUV) * shrink_ratio;

	// // ずらしたuvを使って色を水滴の中の色を持っておく
	// float4 colInWater = tex.Sample(smp, distortionUV);
	// // 水滴の外の色も持っておく
	// float4 colOutWater = tex.Sample(smp, uv);

	// // 縁の近くだけ色を暗くしたいので縁で1、内側だと0になるような数値を作る
	// // 縁の方を1にすると最終的に色にかけたときに縁が真っ暗になるので
	// // 最低限の明るさをかけることでそもそも0.5~1という範囲を作る
	// float waterDropRim = saturate(1.0f - waterToFrameRatio / water_drop_rim_width) * water_drop_rim_darkness;

	// // 縁よりも外は普通の景色にしたい
	// if (waterToFrameRatio <= 0.0f)
	// {
	// 	waterDropRim = 0.0f;
	// }

	// // その二つを縁に近づくにつれて色が混ざるようにする
	// // waterToFrameRatioは水滴の外に行くにつれて0になる
	// float4 col = lerp(colOutWater, colInWater, waterToFrameRatio);

	// // 水滴に光を当てたいので光の位置を決める
	// // ベクトルの長さは半径を超えないようにする
	// float2 waterDropBrightnessPos = float2(
	// 	water_drop_radius * water_drop_highlight_offset,
	// 	-water_drop_radius * water_drop_highlight_offset);
	// // 光の位置から今回のピクセルまでの距離を計算
	// // 使用するuv値は光の位置がずれないように補正後を使用する
	// float brightnessToWaterCenterLength = length(waterDropBrightnessPos - correctedWaterDropToUV);

	// // 光る点の位置では1、縁では0になる値を作る
	// float highlightRatio = saturate(1.0f - brightnessToWaterCenterLength / water_drop_brightness_radius);

	// // 縁の近くの色だけ暗くする必要があるがそのままwaterDropRimをかけると
	// // 縁は普通の色、中が暗くなってしまうので1から引いた値をrgbのみにかける
	// float4 waterDropCol = float4(col.rgb * (1.0f - waterDropRim), col.a);
	// return lerp(waterDropCol,float4(1.0f,1.0f,1.0f,1.0f),highlightRatio);

	// 元のuv
	float2 uv = input.uv;

	for (int i = 0; i < max_water_drop_count; i++)
	{
		WaterDropData drop = waterDrops[i];
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
		float waterDropDarknessRatio = saturate(1.0f - correctedWaterDropToUVLength / water_drop_rim_width * water_drop_rim_darkness);
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
			-drop.radius * water_drop_highlight_offset);

		// 光から今回のピクセルまでの距離を計算
		float lightToFrameLength = length(waterLightPos - correctedWaterDropToUV);

		//光の半径
		float water_drop_light_radius = drop.radius * water_drop_light_radius_ratio;

		// 光の中心が1、光の縁が0になるような数値を作る
		float lightRatio = saturate(1.0f - lightToFrameLength / water_drop_light_radius);

		// 水滴部分以外の色
		float4 colOutWaterDrop = tex.Sample(smp, uv);
		// 水滴部分の色
		float4 colInWaterDrop = tex.Sample(smp, distortionUV);
		// 二つの間の縁周りだけ混ぜる
		float4 waterDropCol = lerp(colOutWaterDrop, colInWaterDrop, waterDropToFrameRatio);
		// 縁だけ暗くする
		waterDropCol.rgb *= waterDropDarknessRatio;
	}

	// 最終的に光から水滴の縁に行くにつれて水滴の色に滑らかに変わっていくようにする
	return lerp(waterDropCol, float4(1.0f, 1.0f, 1.0f, 1.0f), lightRatio);
}
