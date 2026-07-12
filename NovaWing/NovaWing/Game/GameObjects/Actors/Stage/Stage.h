#pragma once
#include "Actor.h"
#include "Manager/ResourceLoader.h"

class CameraBase;
class Stage : public Actor
{
public:
    Stage(ResourceLoader::ModelID modelID,
    std::weak_ptr<CameraBase> pCamera);

    void OnInit() override;
    void Update() override;
    void Draw() override;

private:
    //ステージのピクセルシェーダハンドル
    int m_stagePSH = -1;
    //ライティング用の頂点シェーダハンドル
    int m_lightVSH = -1;
    //シェーダに渡す光の情報
    struct LightingBuffer
    {
        Vector3 lightVec;
        float padding;//16バイトアライメント
    };
    //定数バッファハンドル
    int m_cBufferLight = -1;
    //定数バッファポインタ
    LightingBuffer* m_pCBuffLightData = nullptr;
};