//ライティング計算を行うときの共通コード
//ライティングの計算結果のlightとspecularを返す

struct LightingResult
{
	float light;
	float specular;
};

//環境光
static const float ambient_light = 0.35f;//環境光の強さ

//法線マップの強調度
static const float normal_map_strength = 1.5f;

//滑らかさ
static const float smoothness_min = 1.0f;//滑らかさの最小値
static const float smoothness_range = 34.0f;//滑らかさの変化させる幅

LightingResult CalcLighting(
	float4 normMapCol,//法線マップの色
	float4 metCol,//メタリックマップの色
	float3 normalWS,//ワールド空間の法線
	float3 tangentWS,//ワールド空間の接線
	float3 lightVec,//光の方向
	float3 cameraPos,//カメラの位置
	float3 worldPos//ピクセルのワールド座標
)
{
	//法線方向は-1～1の範囲なので変換する(タンジェント空間)
    float3 normalTS = normMapCol * 2.0 - 1.0;
    //凹凸がしょぼいのでタンジェント空間の法線を強めにする
    normalTS.xy *= normal_map_strength;

	//法線を正規化(ワールド空間)
    float3 normVec = normalize(normalWS);
    
    //正規化した接線(ワールド空間)
    float3 normTangent = normalize(tangentWS);
    
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
    float lightStrength = saturate(diffuse + ambient);

	//カメラからピクセルへの視線方向を求める
    float3 viewDir = normalize(cameraPos - worldPos);
    //反射ベクトルを求める
    //normLightVecは物体→光源なので
    //-で反転させる
    float3 reflectVec = reflect(normLightDir, normalWSFinal);
    //滑らかさを計算
    float smoothness = metCol.r * smoothness_range + smoothness_min;//1～30
    
    //スペキュラの計算
    float specular = pow(saturate(dot(viewDir, reflectVec)), smoothness);
    specular *= diffuse;

	//構造体に結果を入れて返す
	LightingResult result;
	result.light = lightStrength;
	result.specular = specular;
	return result;
}