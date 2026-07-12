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

float4 main(PS_INPUT input) : SV_TARGET
{
	//法線を正規化
	float3 normVec = normalize(input.normalWS);
	//ライトの方向を正規化
	float3 normLightVec = normalize(lightVec);
	//明るさを算出
	float diffuse = saturate(dot(normVec,-normLightVec));
	//環境光
	float ambient = 0.2f;
	//最終的な明るさ
	float light = saturate(diffuse + ambient);
	//ベースの色
	float4 baseCol = float4(5.0f,1.0f,1.0f,1.0f);
	//明るさをかけて出す
	return baseCol * light;
}