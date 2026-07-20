Texture2D<float4> uiTex : register(t0);
SamplerState smp : register(s0);

struct PS_Input
{
	float4 pos : SV_Position;
	float2 uv : TEXCOORD0;
};

static const float scanline_frequency = 300.0f;//スキャンラインを入れる間隔
static const float scanline_strength = 0.3f;//最大でどれぐらい明るさが落ちるか

float4 main(PS_Input input) : SV_TARGET
{
	//ベースのカラー
	float4 baseCol = uiTex.Sample(smp,input.uv);

	//横線(スキャンライン)を計算
	//uvにスキャンラインを入れる間隔をかける
	float scanline = sin(input.uv.y * scanline_frequency);//-1~1
	//それだけでは明るさとして使えないので、0~1に正規化する
	scanline = scanline * 0.5f + 0.5f;

	//スキャンラインの明るさをベースの色にかける
	//普通に書けると、scanlineが0のところは真っ黒になってしまうので、
	//少し弱くなるようにsccanlineをlerpで補間する
	//最小で1.0f-scanline_strengthになる
	float brightnessScanline = lerp(1.0f,scanline,scanline_strength);//0.7～1.0になる
	float3 finalCol = baseCol * brightnessScanline;
	return float4(finalCol,baseCol.a);
}