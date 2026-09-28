Texture2D<float4> skyFront : register(t0);
Texture2D<float4> skyBack : register(t1);
Texture2D<float4> skyRight : register(t2);
Texture2D<float4> skyLeft : register(t3);
Texture2D<float4> skyUp : register(t4);
Texture2D<float4> skyBottom : register(t5);
Texture2D<float4> sceneCapture : register(t6);
Texture2D<float4> causticsTex : register(t7);

SamplerState smp : register(s0);

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
    float screenWidth;//画面の幅
    float screenHeight;//画面の高さ
    float3 padding;
}

cbuffer WaterBuffer : register(b6)
{
    float time;//時間管理
    float3 padding2;
}

float3 SampleSkyReflection(float3 reflectVec)
{
    //反射ベクトルは立方体の中心から外側に向かって伸びているベクトル
    //このベクトルがどの面を貫くか判定する
    //全ての成分の絶対値を比較する
    float absX = abs(reflectVec.x);
    float absY = abs(reflectVec.y);
    float absZ = abs(reflectVec.z);
    
    //xとyどちらが大きいか
    float maxXY = max(absX, absY);
    //それとzを比べてどちらが大きいか
    float maxAll = max(maxXY, absZ);
    //その値がどの成分なのか調べるためにそれぞれと照合する
    if (maxAll == absX)//X
    {
        //他の成分がその面のどのあたりにあるのかを
        //調べるためにscaleを逆算で求める
        float scale = 1.0f / absX;
        //面上のx以外の座標を求める
        float faceY = reflectVec.y * scale;
        float faceZ = reflectVec.z * scale;
        
        //左右どちらなのかを調べる
        if (reflectVec.x > 0.0f)//右
        {
            //右面の左上のuv = (0,0)
            //右上のuv = (1,0)
            float faceU = 1.0f - (faceZ + 1.0f) / 2; //0～1
            float faceV = 1.0f - (faceY + 1.0f) / 2; //0～1
            
            //右面のテクスチャをサンプリングしてその色を返す
            float3 skyColor = skyRight.Sample(smp, float2(faceU, faceV)).rgb;
            return float3(skyColor);
        }
        else //左
        {
            float faceU = (faceZ + 1.0f) / 2; //0～1
            float faceV = 1.0f - (faceY + 1.0f) / 2; //0～1
            
            //左面のテクスチャをサンプリングしてその色を返す
            float3 skyColor = skyLeft.Sample(smp, float2(faceU, faceV)).rgb;
            return float3(skyColor);
        }
    }
    else if (maxAll == absY)//Y
    {
        //他の成分がその面のどのあたりにあるのかを
        //調べるためにscaleを逆算で求める
        float scale = 1.0f / absY;
        //面上のx以外の座標を求める
        float faceX = reflectVec.x * scale;
        float faceZ = reflectVec.z * scale;
        
        //上下どちらなのかを調べる
        if (reflectVec.y > 0.0f)//上
        {
            float faceU = (faceX + 1.0f) / 2; //0～1
            float faceV = (faceZ + 1.0f) / 2; //0～1
            
            //上面のテクスチャをサンプリングしてその色を返す
            float3 skyColor = skyUp.Sample(smp, float2(faceU, faceV)).rgb;
            return float3(skyColor);
        }
        else //下
        {
            float faceU = (faceX + 1.0f) / 2; //0～1
            float faceV = 1.0f - (faceZ + 1.0f) / 2; //0～1
            
            //下面のテクスチャをサンプリングしてその色を返す
            float3 skyColor = skyBottom.Sample(smp, float2(faceU, faceV)).rgb;
            return float3(skyColor);
        }
    }
    else //Z
    {
         //他の成分がその面のどのあたりにあるのかを
        //調べるためにscaleを逆算で求める
        float scale = 1.0f / absZ;
        //面上のz以外の座標を求める
        float faceX = reflectVec.x * scale;
        float faceY = reflectVec.y * scale;
        
        //前後どちらなのかを調べる
        if (reflectVec.z > 0.0f)//前
        {
            float faceU = (faceX + 1.0f) / 2; //0～1
            float faceV = 1.0f - (faceY + 1.0f) / 2; //0～1
            
            //前面のテクスチャをサンプリングしてその色を返す
            float3 skyColor = skyFront.Sample(smp, float2(faceU, faceV)).rgb;
            return float3(skyColor);
        }
        else //後ろ
        {
            float faceU = 1.0f - (faceX + 1.0f) / 2; //0～1
            float faceV = 1.0f - (faceY + 1.0f) / 2; //0～1
            
            //背面のテクスチャをサンプリングしてその色を返す
            float3 skyColor = skyBack.Sample(smp, float2(faceU, faceV)).rgb;
            return float3(skyColor);
        }
    }
}

//カメラのnearとfar
static const float near_clip = 200.0f;
static const float far_clip = 5500.0f;

//海の色(ベースの色)
static const float3 shallow_color = float3(0.0f, 0.5f, 0.9f);
static const float3 deep_color  = float3(0.0f, 0.05f, 0.2f);

//泡
static const float foam_height_min = 50.0f;//泡が出始める高さ
static const float foam_height_max = 130.0f;//完全に泡になる高さ

//霧
static const float fog_dist_min = 800.0f;//霧がかかり始める距離
static const float fog_dist_max = 2000.0f;//完全に霧になる距離

//スペキュラ(反射光)
static const float specular_power = 30.0f;//ハイライトの鋭さ
static const float specular_strength = 0.5f;//反射光の強さ

//アンビエント(環境光)
static const float ambient_light = 0.2f;

//フレネル効果
static const float fresnel_power = 20.0f;//フレネルの強さ

//reveal(水面下の物体を透かす処理)
static const float reveal_alpha_threshold = 0.001f;//背景判定用の閾値
static const float reveal_delta_range = 0.15f;//深さの差の許容範囲
static const float reveal_strength = 0.6f;//revealの最大強度

//コースティクス(海の模様)
static const float caustics_uv_scale = 0.001f;//テクスチャの広がり方
static const float caustics_flow_speed = 0.1f;//揺れる速さ
static const float caustics_dist_min = 1500.0f;//効果が弱まり始める距離
static const float caustics_dist_max = 3000.0f;//効果が消える距離

float4 main(PS_INPUT input) : SV_TARGET
{
    //視線方向を計算
    float3 viewDir = normalize(cameraPos - input.worldPos);

    //光のベクトルを計算
    float3 normLightDir = normalize(lightVec);
    
    //環境光を追加
    float ambient = ambient_light;
    //法線と光のベクトルの内積から暗い部分と明るい部分を出す
    float diffuse = saturate(dot(input.normalWS, -normLightDir));
    //最終的な光の強さ
    float light = saturate(diffuse + ambient);
    
    //光の反射用のベクトル(specular用)
    float3 lightRefVec = reflect(normLightDir, input.normalWS);
    //反射光の計算
    //少しまぶしいので値を小さくする
    float specular = pow(saturate(dot(
        viewDir, lightRefVec)), specular_power) * specular_strength;
    
    //空を海に反射させるために
    //視線ベクトルとワールド座標から反射ベクトルを計算
    //今回はカメラ→水面ではなく
    //水面→カメラのベクトルが欲しいので既にある
    //視線ベクトルを反転させる
    float3 viewRefVec = reflect(-viewDir, input.normalWS);
    
    //空の色を求める
    float3 skyColor = SampleSkyReflection(viewRefVec);

    //フレネル効果
    //水面を真上から見ると透明
    //水面を斜めから見ると反射が強くなる
    //これを活かして透明感と深みを表現する
    //視線方向と法線の角度を使用する
    float fresnel = pow(
    1.0f - saturate(dot(viewDir, input.normalWS)), fresnel_power);
    
    float3 waterColor = lerp(
    shallow_color, deep_color, fresnel);
    
    //求めた空の色と水の色をフレネルで補間する　
    float3 finalWaterCol = lerp(waterColor, skyColor, fresnel);
    
    //白波を表現する
    //波の頂点付近(Y座標が高い場所)を白くする
    float waveHeight = input.worldPos.y;
    //泡の部分
    //smoothstep(min,max,x)は、
    //min以下は0,max以上は1
    //その間は滑らかに0～1へ変化する
    float foam = smoothstep(foam_height_min, foam_height_max, waveHeight);
    //海の色(空の反射も混ざっている色)と白を混ぜる
    //lerpで泡の部分は白に補間する
    float3 finalColor = lerp(finalWaterCol.rgb, float3(1, 1, 1), foam);
    
    //最終的な見た目に霧をかけたいので先に
    //ライティングと反射光を適用した後の色を出しておく
    float3 litColor = finalColor * light + specular;
    
     //霧(水平線を表現する)
    //カメラから水面までの距離を求める
    float dist = length(cameraPos - input.worldPos);
    //距離に応じた霧の強さ(0～1)
    float fogFactor = smoothstep(fog_dist_min, fog_dist_max, dist);
    
    //最終的な色に霧を適用する
    float3 foggedColor = lerp(litColor, skyColor, fogFactor);
    
    //スクリーンUV座標を求める
    //input.posはSV_POSITIONセマンティクスで、ピクセルシェーダに渡された時点ではすでに
    //ピクセル単位のスクリーン座標になっている(0～画面幅、高さ)これをwidth / screenHeightで割ることで
    //0～1のUV座標に変換できる
    float2 screenUV = input.pos.xy / float2(screenWidth,screenHeight);
    //シーンをキャプチャしてきたテクスチャをそのUV座標でサンプリング
    float4 captureCol =  sceneCapture.Sample(smp,screenUV);

    //アルファがほぼ0の場所はrevealを0にする
    //何もない場所でたまたまdeltaが小さくなり、不自然に水面が薄くなってしまう可能性があるため
    float reveal = 0.0f;
    if(captureCol.a > reveal_alpha_threshold)
    {
        //水面自体の正規化距離を求める
        float normDistNtoF = saturate((dist - near_clip) / (far_clip - near_clip));
        //物体までの距離と水面までの距離を引き算して、
        //その差が小さいほど水面のすぐ下に物体があると判定できる
        //絶対値にしなければ広い範囲で効果が出てしまう
        float delta =  abs(captureCol.a - normDistNtoF);
        //smoothstep(0.0f,0.05f,delta)で出るのは、deltaが小さいほど0になってしまうので
        //それを1.0 - にすることでdeltaが小さいほど透明度が1、deltaが大きいほど透明度が0になる
        reveal = (1.0f - smoothstep(0.0f, reveal_delta_range, delta)) * reveal_strength;
    }
    
    //ワールド座標をUVに変換する
    //worldPosにかける値が大きいほどテクスチャが粗く広がる
    //小さいほど細かく繰り返す
    float2 causticsUV = input.worldPos.xz * caustics_uv_scale;
    //距離に応じてコースティクスの強度を変えるための係数
    //1.0から引くことで手前の方が強度が高くなる
    float distanceStrength = 1.0f - smoothstep(caustics_dist_min,caustics_dist_max,dist);
    
    //時間に合わせて2枚のUVをずらして揺れているように見せる
    float2 causticsUV1 = causticsUV + time * float2(caustics_flow_speed, caustics_flow_speed);
    float2 causticsUV2 = causticsUV + time * float2(-caustics_flow_speed, caustics_flow_speed);
    
    //UVから色をサンプリングする
    float4 baseCausticsCol1 = causticsTex.Sample(smp, frac(causticsUV1));
    float4 baseCausticsCol2 = causticsTex.Sample(smp, frac(causticsUV2));
    //コースティクスのテクスチャがアルファ値に網目模様が入っているので
    //アルファのみを合成する
    float causticsAlpha = baseCausticsCol1.a * baseCausticsCol2.a;

    //距離が近い海面ほどコースティクス効果を出す
    //アルファに模様が入っているので、まず白を作ってから
    //アルファ値をかけて模様を作成
    float4 causticsFinal = float4(1.0f,1.0f,1.0f,1.0f) * causticsAlpha * distanceStrength;

    //霧を適用させたカラーと、キャプチャーしたカラーをrevealで補間する
    float3 finalCol = lerp(foggedColor,captureCol.rgb, reveal);
    //最終的な色にコースティクス効果を付け足す
    finalCol += causticsFinal.rgb;
    
    //不透明で返す
    return float4(finalCol,1.0f);
}