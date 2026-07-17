#include "Stage.h"
#include "Manager/LightingManager.h"
#include "Constants/ShaderRegister.h"

namespace
{
    const Vector3 model_scale = Vector3(0.4f,0.4f,0.4f);
    //ステージの初期位置
    const Vector3 first_pos = Vector3(0.0f,0.0f,17700.0f);
    //光の方向ベクトル
    const Vector3 light_dir = Vector3(0.0f,-0.5f,-1.0f);
}

Stage::Stage(ResourceLoader::ModelID modelID, std::weak_ptr<CameraBase> pCamera):
    Actor(modelID,pCamera)
{
    
}

void Stage::OnInit()
{
    //ステージの位置を設定
    SetPos(first_pos);
    //カメラと行列情報定数バッファを作成
    CreateShaderBuffers();
    //光情報の定数バッファ作成
    m_cBufferLight = CreateShaderConstantBuffer(sizeof(LightingBuffer));
    m_pCBuffLightData = static_cast<LightingBuffer*>(GetBufferShaderConstantBuffer(m_cBufferLight));

    //シェーダハンドルを読み込む
    m_stagePSH =  LoadPixelShader(L"StagePS.pso");
    m_lightVSH = LoadVertexShader(L"LightingVS.vso");
    //光の向きをセットする
    m_pCBuffLightData->lightVec = light_dir;
    //定数バッファを更新する
    UpdateShaderConstantBuffer(m_cBufferLight);
}

void Stage::Update()
{
    //処理なし   
}

void Stage::Draw()
{
    //行列適用
    ApplyMatrix(model_scale,m_pos,m_rotation,m_modelHandle);
    UpdateShaderMatrixData();
    //自前のシェーダを使うことを宣言
    MV1SetUseOrigShader(true);
    //シェーダをセットする
    SetUsePixelShader(m_stagePSH);
    SetUseVertexShader(m_lightVSH);
    //定数バッファをセットする
    SetShaderConstantBuffer(m_cBufferLight,DX_SHADERTYPE_PIXEL,ShaderRegister::stage_cbuffer_light);
    BindShaderBuffers();
   
    //モデルを描画する
    MV1DrawModel(m_modelHandle);

    //自前のシェーダーを一度解除
    MV1SetUseOrigShader(false);
    //シェーダを解除
    SetUsePixelShader(-1);
    SetUseVertexShader(-1);
    //定数バッファも解除する
    SetShaderConstantBuffer(-1,DX_SHADERTYPE_PIXEL,ShaderRegister::stage_cbuffer_light);
    ReleaseShaderBuffers();
}
