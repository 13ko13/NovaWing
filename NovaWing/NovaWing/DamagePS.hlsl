Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

struct PS_INPUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

float4 main() : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}