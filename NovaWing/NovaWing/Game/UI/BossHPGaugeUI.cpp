#include "BossHPGaugeUI.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/BossEnemy/BossEnemy.h"
#include "Manager/ResourceLoader.h"
#include "Utility/SizeF.h"
#include "Constants/ShaderRegister.h"
#include "Utility/Size.h"
#include "Utility/GraphShaderDraw.h"
#include "Utility/Vector2.h"
#include "Main/Application.h"

namespace
{
    //HP枠画像の位置
    const Vector2 hp_frame_pos_ratio = { 0.03f,0.2f };
    //スキャンラインを入れる周期
    constexpr float scanline_frequency = 655.0f;
}

BossHPGaugeUI::BossHPGaugeUI(std::weak_ptr<BossEnemy> pBoss) :
    HPGaugeUIBase(pBoss),
    m_pBoss(pBoss)
{
    //スキャンラインを入れる周期をシェーダに渡す
    m_pCBuffGlitchData->scanlineFrequency = scanline_frequency;
    UpdateShaderConstantBuffer(m_cbufferGlitch);
}

BossHPGaugeUI::~BossHPGaugeUI()
{

}

void BossHPGaugeUI::Update()
{
    HPGaugeUIBase::Update();
}

void BossHPGaugeUI::Draw()
{
    //ボスの出現が完了していたら描画する
    if (m_pBoss.lock()->IsBossAppear())
    {
        //HPの枠画像を取得
        int hpFrameHandle = ResourceLoader::GetInstance().GetGraphic(
            ResourceLoader::GraphicID::BossHPFrame
        );
        //HPゲージの画像を取得
        int hpGaugeHandle = ResourceLoader::GetInstance().GetGraphic(
            ResourceLoader::GraphicID::BossHPGauge
        );

        //ウィンドウサイズ
        const Size& wsize = Application::GetInstance().GetWindowSize();
        Vector2 frameDrawPos = Vector2(
            wsize.m_width * hp_frame_pos_ratio.m_x,
            wsize.m_height * hp_frame_pos_ratio.m_y
        );

        //どちらもシェーダを通して描画する
        DrawHPGauge(hpFrameHandle, hpGaugeHandle, frameDrawPos,true);
    }

#ifdef _DEBUG

#endif
}