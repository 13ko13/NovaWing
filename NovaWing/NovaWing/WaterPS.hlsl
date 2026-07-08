Texture2D skyFront : register(t0);
Texture2D skyBack : register(t1);
Texture2D skyRight : register(t2);
Texture2D skyLeft : register(t3);
Texture2D skyUp : register(t4);
Texture2D skyBottom : register(t5);

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

//float3 SampleSkyReflection(float3 reflectVec)
//{
//    //反射ベクトルは立方体の中心から外側に向かって伸びているベクトル
//    //このベクトルがどの面を貫くか判定する
//    //全ての成分の絶対値を比較する
//    float absX = abs(reflectVec.x);
//    float absY = abs(reflectVec.y);
//    float absZ = abs(reflectVec.z);
    
//    //xとyどちらが大きいか
//    float maxXY = max(absX, absY);
//    //それとzを比べてどちらが大きいか
//    float maxAll = max(maxXY, absZ);
//    //その値がどの成分なのか調べるためにそれぞれと照合する
//    if (maxAll == absX)//X
//    {
//        //他の成分が
//        //反射ベクトルを1にするためのscaleを計算      
//        float scale = 1.0f / reflectVec.x;
//        //左右どちらなのかを調べる
//        if (reflectVec.x > 0.0f)//右
//        {
            
//        }
//        else //左
//        {
            
//        }
//    }
//    else if (maxAll == absY)//Y
//    {
//        //上下どちらなのかを調べる
//        if (reflectVec.y > 0.0f)//上
//        {
            
//        }
//        else //下
//        {
            
//        }
//    }
//    else //Z
//    {
//        //前後どちらなのかを調べる
//        if (reflectVec.z > 0.0f)//前
//        {
            
//        }
//        else //後ろ
//        {
            
//        }
//    }
//}

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
    //min以下は0,max以上は1
    //その間は滑らかに0～1に補間
    float foam = smoothstep(90.0f, 110.505f, waveHeight);
    //海の色と白を混ぜる
    //lerpで泡の部分は白に補間する
    float3 finalColor = lerp(waterColor.rgb, float3(1, 1, 1), foam );
    
    //テクスチャの色に明るさを適用してそのピクセルの色を返す
    return float4(finalColor * light + specular, 1.0f);
}