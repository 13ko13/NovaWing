Texture2D skyFront : register(t0);
Texture2D skyBack : register(t1);
Texture2D skyRight : register(t2);
Texture2D skyLeft : register(t3);
Texture2D skyUp : register(t4);
Texture2D skyBottom : register(t5);

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
    float padding;
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
    
    //光の反射用のベクトル(specular用)
    float3 lightRefVec = reflect(normLightDir, input.normalWS);
    
    //空を海に反射させるために
    //入射ベクトルとワールド座標から反射ベクトルを計算
    //今回はカメラ→水面ではなく
    //水面→カメラのベクトルが欲しいので既にある
    //視線ベクトルを反転させる
    float3 viewRefVec = reflect(-viewDir, input.normalWS);
    
    //空の色を求める
    float3 skyColor = SampleSkyReflection(viewRefVec);

    //反射光の計算
    //少しまぶしいので値を小さくする
    float specular = pow(saturate(dot(viewDir, lightRefVec)), 30.0f) * 0.5f;
    
    //フレネル効果
    //水面を真上から見ると透明
    //水面を斜めから見ると反射が強くなる
    //これを活かして透明感と深みを表現する
    //視線方向と法線の角度を使用する
    float fresnel = pow(
    1.0f - saturate(dot(viewDir, input.normalWS)), 20.0f);
    
    //浅い色
    float3 shallowColor = float3(0.0f, 0.5f, 0.9f);
    //深い色
    float3 deepColor = float3(0.0f, 0.05f, 0.2f);
    //二つを組み合わせてフレネル効果を付けた水の色
    float3 waterColor = lerp(
    shallowColor, deepColor, fresnel);
    
    //求めた空の色と水の色をフレネルで補間する　
    float3 finalWaterCol = lerp(waterColor, skyColor, fresnel);
    
    //白波を表現する
    //波の頂点付近(Y座標が高い場所)を白くする
    float waveHeight = input.worldPos.y;
    //泡の部分
    //smoothstep(min,max,x)は、
    //min以下は0,max以上は1
    //その間は滑らかに0～1に補間
    float foam = smoothstep(50.0f, 130.0f, waveHeight);
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
    float fogFactor = smoothstep(800.0f, 2000.0f, dist);
    
    //最終的な色に霧を適用する
    float3 foggedColor = lerp(litColor, skyColor, fogFactor);
    
    //テクスチャの色に明るさを適用してそのピクセルの色を返す
    return float4(foggedColor, 1.0f);
}