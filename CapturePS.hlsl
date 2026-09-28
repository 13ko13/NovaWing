
Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

struct PS_INPUT 
{
	float4 pos : SV_Position;
	float2 uv : TEXCOORD0;
	float3 normalWS : NORMAL;
	float3 worldPos : TEXCOORD1;

	float3 tangentWS : TANGENT;
};

cbuffer CameraBuffer : register(b6)
{
	//カメラの位置
	float3 cameraPos;
	float dummy;//16バイトアライメント
}

static const float near_clip = 200.0f;
static const float far_clip = 5500.0f;

float4 main(PS_INPUT input) : SV_TARGET
{
	//物体の色を取得
	float4 baseColor = tex.Sample(smp,input.uv);

	//カメラから各頂点のワールド座標までの距離を計算する
	//その距離をnear_clip～far_clipの範囲で0~1に正規化
	//near未満は0,far以上は1とする
	float dist = length(cameraPos - input.worldPos);
	//画面に不透明なオブジェクトが映っている前提なのでdist - near_clipでdistがnearからどれぐらい離れた位置にいるかを測る
	//その後far-nearの値(farとnearの間がどれぐらいか)で割るとNearからFarの間で
	//どのぐらいの位置にいるかというのをsaturateすることで0～1で表現できる
	float depthNearToFar = saturate((dist - near_clip) / (far_clip - near_clip));

	//その0～1の値をアルファ値として使用して
	//通常カラーをカメラからの距離によって透明化させた色を返す
	return float4(baseColor.rgb,depthNearToFar);
}