#include "Rock.h"
#include "Game/GameObjects/GameObject.h"
#include "Manager/LightingManager.h"
#include "Manager/ResourceLoader.h"

namespace
{
    const Vector3 model_scale = Vector3(3.0f, 5.0f, 3.0f);
}

Rock::Rock(ResourceLoader::ModelID modelId,
    std::weak_ptr<CameraBase> pCamera) :
    Actor(modelId,pCamera)
{

}

void Rock::OnInit()
{
    //位置の調整
    //仮で0,0,0
    SetPos(Vector3(0.0f, 0.0f, 900.0f));

    //定数バッファを作成
    CreateShaderBuffers();
}

void Rock::Update()
{
}

void Rock::Draw()
{
    //行列適用
    ApplyMatrix(model_scale, m_pos, m_rotation, m_modelHandle);

    //シェーダー渡す行列の更新
    UpdateShaderMatrixData();

    //ロード済みのテクスチャのハンドルを受け取る
    int normHandle = ResourceLoader::GetInstance().GetGraphic(ResourceLoader::GraphicID::RockNorm);
    //テクスチャをシェーダにセットする
    SetUseTextureToShader(1, normHandle);
    //ほかのテクスチャはないので、明示的にないことを伝える
    SetUseTextureToShader(2, -1);
    SetUseTextureToShader(3, -1);

    //ライティングを適用する
    LightingManager::GetInstance().ApplyShader();

    //定数バッファをシェーダにセットする
    BindShaderBuffers();

    //モデルを描画
    MV1DrawModel(m_modelHandle);

    //それぞれを解除する
    SetUseTextureToShader(1, -1);
    LightingManager::GetInstance().ResetShader();
    ReleaseShaderBuffers();
}
