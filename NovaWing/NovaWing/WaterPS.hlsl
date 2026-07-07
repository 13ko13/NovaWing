struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normalWS : NORMAL; //ワールド空間の法線
    float3 worldPos : TEXCOORD1;
};

cbuffer LightingBuffer : register(b3)
{
    float3 lightVec;//光の方向ベクトル
}

cbuffer CameraBuffer : register(b5)
{
    float3 cameraPos;//カメラの位置
    float padding;
}

float4 main(PS_INPUT input) : SV_TARGET
{
    //視線方向を計算
    float3 viewDir = normalize(cameraPos - input.worldPos);

    //光のベクトルを計算
    float3 normLightDir = normalize(lightVec);
    
    //return float4(normalize(lightVec) * 0.5 + 0.5, 1.0f);
    
    //環境光を追加
    float ambient = 0.2f;
    //法線と光のベクトルの内積から暗い部分と明るい部分を出す
    float diffuse = saturate(dot(input.normalWS, -normLightDir));
    //最終的な光の強さ
    float light = saturate(diffuse + ambient);
    
    //視線方向と光の方向から反射ベクトルを計算
    float3 reflectVec = reflect(normLightDir, input.normalWS);

    //反射光の計算
    //少しまぶしいので値を小さくする
    float specular = pow(saturate(dot(viewDir, reflectVec)), 60.0f) * 0.6f;
    
    //フレネル効果
    //水面を真上から見ると透明
    //水面を斜めから見ると反射が強くなる
    //これを活かして透明感と深みを表現する
    //視線方向と法線の角度を使用する
    float fresnel = pow(
    1.0f - saturate(dot(viewDir, input.normalWS)), 5.0f);
    
    //浅い色
    float3 shallowColor = float3(0.0f, 0.5f, 0.9f);
    //深い色
    float3 deepColor = float3(0.0f, 0.05f, 0.2f);
    //最終的な水の色
    float3 waterColor = lerp(
    shallowColor, deepColor, fresnel);
    
    //return float4(input.worldPos.y / 50.0f, 0.0f, 0.0f, 1.0f);
    
    //白波を表現する
    //波の頂点付近(Y座標が高い場所)を白くする
    float waveHeight = input.worldPos.y;
    //泡の部分
    //smoothstep(min,max,x)は、
    //minとmax以内であれば1それ以外は0を返す
    float foam = smoothstep(95.0f, 107.505f, waveHeight);
    //海の色と白を混ぜる
    //lerpで泡の部分は白に補間する
    float3 finalColor = lerp(waterColor.rgb, float3(1, 1, 1), foam * 0.15f);
    
    //テクスチャの色に明るさを適用してそのピクセルの色を返す
    return float4(finalColor * light + specular, 1.0f);
}