#include "Rock.h"
#include "Game/GameObjects/GameObject.h"
#include "Manager/LightingManager.h"
#include "Manager/ResourceLoader.h"
#include "Constants/ShaderRegister.h"

namespace
{
    const Vector3 model_scale = Vector3(3.0f, 5.0f, 3.0f);
}

Rock::Rock(ResourceLoader::ModelID modelId,
    std::weak_ptr<CameraBase> pCamera,
    const Vector3& pos) :
    Actor(modelId,pCamera)
{
    //位置を反映
    SetPos(pos);
}

void Rock::OnInit()
{
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
    int normHandle = ResourceLoader::GetInstance().GetGraphic(
        ResourceLoader::GraphicID::RockNorm
    );
    int noiseHandle = ResourceLoader::GetInstance().GetGraphic(
        ResourceLoader::GraphicID::DissolveNoise
    );
    //ハンドルとレジスタ番号をセットで保持しておく
    std::vector<std::pair<int,int>> textures;
    textures.push_back({ShaderRegister::tex_normal,normHandle});//法線マップ
    textures.push_back({ShaderRegister::tex_metalic,-1});//メタリックマップはないので-1
    textures.push_back({ShaderRegister::tex_emission,-1});//エミッションも同じ
    textures.push_back({ShaderRegister::tex_noise,noiseHandle});//ノイズテクスチャ

    //DrawWithLightingにペアの配列を渡してモデルを描画
    DrawWithLighting(textures);
}
