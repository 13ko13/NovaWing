#include <DxLib.h>
#include <cassert>

#include "HPGaugeUI.h"
#include "Manager/ResourceLoader.h"
#include "Game/GameObjects/Actors/Charactor/Player/Player.h"
#include "Utility/Vector2.h"
#include "Utility/SizeF.h"
#include "Constants/ShaderRegister.h"
#include "Utility/Size.h"
#include "Utility/GraphShaderDraw.h"

namespace
{
    //HP枠画像
    constexpr double hp_frame_size = 0.3;//HP枠画像の大きさ
    const Vector2 hp_frame_pos = { 30.0f,30.0f };//HP枠画像の位置

    //HPゲージ画像
    constexpr double hp_gauge_size = 0.3;//HPゲージの大きさ
}

HPGaugeUI::HPGaugeUI(std::weak_ptr<Player> pPlayer) :
    m_pPlayer(pPlayer)
{
    //グリッチシェーダをロード
    m_glitchPSH = LoadPixelShader(L"GlitchPS.pso");
    assert(m_glitchPSH >= 0);
}

void HPGaugeUI::Draw()
{
    //HPの枠画像を取得
    int hpFrameHandle = ResourceLoader::GetInstance().GetGraphic(
        ResourceLoader::GraphicID::HPFrame
    );
    //HPゲージの画像を取得
    int hpGaugeHandle = ResourceLoader::GetInstance().GetGraphic(
        ResourceLoader::GraphicID::HPGauge
    );

    //グリッチシェーダを適用
    SetUsePixelShader(m_glitchPSH);
    
    //枠の画像サイズを取得
    Size frameSize;
    GetGraphSize(hpFrameHandle, &frameSize.m_width, &frameSize.m_height);
    SizeF frameSizeF = {
        static_cast<float>(frameSize.m_width) * static_cast<float>(hp_frame_size),
        static_cast<float>(frameSize.m_height) * static_cast<float>(hp_frame_size)
    };

    //枠の画像を描画
    DrawGraphToShader(
        hp_frame_pos.m_x,
        hp_frame_pos.m_y,
        frameSizeF, 1.0f,
        hpFrameHandle
    );

    //プレイヤーのshared_ptrを取得
    std::shared_ptr<Player> pPlayer =  m_pPlayer.lock();

    //HPの割合から、切り取り位置を計算
    float ratio =
        static_cast<float>(pPlayer->GetHealth()) /
        static_cast<float>(pPlayer->GetMaxHealth());

    //HPゲージ画像の大きさを取得
    Size gaugeSize;
    GetGraphSize(hpGaugeHandle, &gaugeSize.m_width, &gaugeSize.m_height);
    SizeF gaugeSizeF = {
        static_cast<float>(gaugeSize.m_width) * static_cast<float>(hp_gauge_size),
        static_cast<float>(gaugeSize.m_height) * static_cast<float>(hp_gauge_size)
    };

    //ゲージ位置を定義(左上座標は枠の場所と同じ)
    Vector2 gaugePos = Vector2(hp_frame_pos.m_x, hp_frame_pos.m_y);

    //ゲージを描画
    DrawGraphToShader(
        gaugePos.m_x, gaugePos.m_y,//位置
        gaugeSizeF,ratio,hpGaugeHandle
    );

    SetUsePixelShader(-1);

#ifdef _DEBUG
    DrawFormatString(0, 515, 0xff0000, L"gaugePosX: %f", gaugePos.m_x);
#endif
}
