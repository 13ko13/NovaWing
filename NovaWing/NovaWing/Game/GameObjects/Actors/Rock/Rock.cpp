#include "Rock.h"
#include "Game/GameObjects/GameObject.h"
#include "Manager/LightingManager.h"
#include "Manager/ResourceLoader.h"
#include "Constants/ShaderRegister.h"

namespace
{
    const Vector3 model_scale = Vector3(3.0f, 5.0f, 3.0f);//モデルのサイズ
}

Rock::Rock(std::weak_ptr<CameraBase> pCamera, const RockData& data):
    Actor(data.modelId, pCamera)
{
    //位置を反映
    SetPos(data.pos);

    //半径が入れられたデータの数分ループを回す(必要な球の数だけ)
    for (int i = 0; i < data.sphereRadii.size();i++)
    {
        //受け取った岩のデータからYオフセットと半径をセットする
        Vector3 posWithOffset = Vector3(
            data.pos.m_x,
            data.pos.m_y + data.sphereYOffsets[i],
            data.pos.m_z
        );

        //その球の半径を受け取ってセットする
        float radius = data.sphereRadii[i];

        Sphere sphere = Sphere(posWithOffset, radius);
        m_spheres.push_back(sphere);
    }
}

void Rock::OnInit()
{
    //定数バッファを作成
    CreateShaderBuffers();
}

void Rock::Update()
{
    //岩は動かないので処理なし
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

#ifdef _DEBUG
    //全ての球を描画
    for (Sphere& sphere : m_spheres)
    {
        sphere.Draw(0xff0000);
    }
#endif // _DEBUG
}
