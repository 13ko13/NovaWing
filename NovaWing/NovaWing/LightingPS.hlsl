Texture2D<float4> tex : register(t0);
Texture2D<float4> normTex : register(t1);
Texture2D<float4> metalicTex : register(t2);
Texture2D<float4> emissionTex : register(t3);
Texture2D<float4> noiseTex : register(t4);

SamplerState smp : register(s0);

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

cbuffer CameraBuffer : register(b6)
{
    float3 cameraPos;
    float padding;
}

//カメラ
static const float near = 200.0f;//カメラのニア

//ディゾルブ
static const float start_disolve = 400.0f;//ディゾルブ開始距離
static const float noise_uv_scale = 100.0f;//ノイズUVの縮小率

//環境光
static const float ambient_light = 0.2f;//環境光の強さ

//法線マップの強調度
static const float normal_map_strength = 1.5f;

//滑らかさ
static const float smoothness_min = 1.0f;//滑らかさの最小値
static const float smoothness_range = 29.0f;//滑らかさの変化させる幅

float4 main(PS_INPUT input) : SV_TARGET
{
    //物体がNearに近づくにつれてディゾルブで消えていくようにする
    //カメラからピクセルまでの距離を求める
    float cameraToPixelD = distance(cameraPos,input.worldPos);
    //この距離がNearに近づくにつれてノイズの閾値をあげていく
    float noise_threshold = 1.0f - smoothstep(near,start_disolve,cameraToPixelD);
    
    //ノイズをサンプリングする
    //岩のuvが変なので、worldPosを使ったuvを作成する
    float2 worldBaseUV = input.worldPos.xy / noise_uv_scale;
    float3 noiseCol = noiseTex.Sample(smp,worldBaseUV);

    //ノイズのカラーの値が閾値よりも小さければそのピクセルの描画を行わない
    if(noiseCol.r < noise_threshold) discard;
    
    //メタリックマップのカラーを取得
    float4 metCol = metalicTex.Sample(smp, input.uv);
    
    //ピクセル情報を取得
    float4 texColor = tex.Sample(smp, input.uv);
    //法線マップのカラーを取得(0～1)
    float4 normMapColor = normTex.Sample(smp, input.uv);
    //エミッション(影の場所でも光る)
    float4 emission = emissionTex.Sample(smp, input.uv);

    //法線方向は-1～1の範囲なので変換する(タンジェント空間)
    float3 normalTS = normMapColor * 2.0 - 1.0;
    //凹凸がしょぼいのでタンジェント空間の法線を強めにする
    normalTS.xy *= normal_map_strength;
    
    //法線を正規化(ワールド空間)
    float3 normVec = normalize(input.normalWS);
    
    //正規化した接線(ワールド空間)
    float3 normTangent = normalize(input.tangentWS);
    
    //ワールド空間のtangentとnormalから外積を使用してbinormalを作る
    //どちらもワールド空間なのでbinormalもワールド空間になる
    float3 binormal = normalize(cross(normVec, normTangent));
    
    //タンジェント空間の法線をワールド空間に変換する
    float3 normalWSFinal = float3(normTangent * normalTS.x +
                                binormal * normalTS.y +
                                normVec * normalTS.z);
    //それを正規化
    normalWSFinal = normalize(normalWSFinal);
    
    //光のベクトルを正規化
    float3 normLightDir = normalize(lightVec);
    // アンビエントを追加（完全な暗闇を防ぐ）
    float ambient = ambient_light;
    
    //その二つの内積を計算
    //内積の結果が1:明るい,0以下:暗い
    //内積は-1も出るが、0以下は光があたっていないのでsaturteで0～1にクランプ
    float diffuse = saturate(dot(normalWSFinal, -normLightDir));
    //最終的な光の強さ
    float light = saturate(diffuse + ambient);
    
    //カメラからピクセルへの視線方向を求める
    float3 viewDir = normalize(cameraPos - input.worldPos);
    //反射ベクトルを求める
    //normLightVecは物体→光源なので
    //-で反転させる
    float3 reflectVec = reflect(normLightDir, normalWSFinal);
    //滑らかさを計算
    float smoothness = metCol.r * smoothness_range + smoothness_min;//1～30
    
    //スペキュラの計算
    float specular = pow(saturate(dot(viewDir, reflectVec)), smoothness);
    specular *= diffuse;
    //拡散色テクスチャのアルファをスペキュラ強度マップとして使用
    //(このシェーダーは元々テクスチャのアルファを使っておらず、出力アルファも1.0f固定なので競合しない。
    //アルファチャンネルを持たないテクスチャはサンプル結果が1.0になるため既存モデルへの影響もない)
    specular *= texColor.a;

    //テクスチャの色に明るさを適用してそのピクセルの色を返す
    return float4(texColor.rgb * light + specular + emission.rgb, 1.0f);
}