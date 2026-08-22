#include <DxLib.h>
#include <cassert>

#include "HPGaugeUIBase.h"
#include "Manager/ResourceLoader.h"
#include "Utility/SizeF.h"
#include "Constants/ShaderRegister.h"
#include "Utility/Size.h"
#include "Utility/GraphShaderDraw.h"
#include "Utility/Vector2.h"
#include "Game/GameObjects/Actors/Charactor/Charactor.h"

namespace
{
    //HP枠画像
    constexpr double hp_frame_size = 0.3;//HP枠画像の大きさ

    //HPゲージ画像
    constexpr double hp_gauge_size = 0.3;//HPゲージの大きさ

    //シェーダにフレームを渡すときに値が大きすぎるので小さくするための値
    constexpr float time_speed = 0.1f;
}

HPGaugeUIBase::HPGaugeUIBase(std::weak_ptr<Charactor> pCharactor):
    m_pHpTargetCharactor(pCharactor)
{
    //グリッチシェーダをロード
    m_glitchPSH = LoadPixelShader(L"GlitchPS.pso");
    assert(m_glitchPSH >= 0);

    //シェーダバッファを作成
    m_cbufferGlitch = CreateShaderConstantBuffer(sizeof(GlitchBuffer));
    m_pCBuffGlitchData = static_cast<GlitchBuffer*>(GetBufferShaderConstantBuffer(m_cbufferGlitch));
}

HPGaugeUIBase::~HPGaugeUIBase()
{
}

void HPGaugeUIBase::Update()
{
    //フレームを更新
    m_frame++;
    //シェーダに時間を渡す
    m_pCBuffGlitchData->time = m_frame * time_speed;
    UpdateShaderConstantBuffer(m_cbufferGlitch);
}

void HPGaugeUIBase::DrawHPGauge(
    int frameHandle,
    int gaugeHandle,
    const Vector2& drawPos,
    bool isBoss
)
{
    //グリッチシェーダを適用
    SetUsePixelShader(m_glitchPSH);
    SetShaderConstantBuffer(m_cbufferGlitch, DX_SHADERTYPE_PIXEL, ShaderRegister::glitch_buffer);

    //枠の画像サイズを取得
    Size frameSize;
    GetGraphSize(frameHandle, &frameSize.m_width, &frameSize.m_height);
    SizeF frameSizeF = {
        static_cast<float>(frameSize.m_width) * static_cast<float>(hp_frame_size),
        static_cast<float>(frameSize.m_height) * static_cast<float>(hp_frame_size)
    };

    //枠の画像を描画
    DrawRectHorizontalGraphToShader(
        drawPos.m_x,
        drawPos.m_y,
        frameSizeF, 1.0f,
        frameHandle
    );

    //プレイヤーのshared_ptrを取得
    std::shared_ptr<Charactor> pCharactor = m_pHpTargetCharactor.lock();

    //HPの割合から、切り取り位置を計算
    float ratio =
        static_cast<float>(pCharactor->GetHealth()) /
        static_cast<float>(pCharactor->GetMaxHealth());

    //HPゲージ画像の大きさを取得
    Size gaugeSize;
    GetGraphSize(gaugeHandle, &gaugeSize.m_width, &gaugeSize.m_height);
    SizeF gaugeSizeF = {
        static_cast<float>(gaugeSize.m_width) * static_cast<float>(hp_gauge_size),
        static_cast<float>(gaugeSize.m_height) * static_cast<float>(hp_gauge_size)
    };

    //ゲージ位置を定義(左上座標は枠の場所と同じ)
    Vector2 gaugePos = Vector2(drawPos.m_x, drawPos.m_y);

    //ゲージを描画
    //ボスじゃない場合
    if (isBoss)
    {
        DrawRectVerticalGraphToShader(
            gaugePos.m_x,
            gaugePos.m_y,//位置
            gaugeSizeF, ratio, gaugeHandle
        );
    }
    else
    {
        DrawRectHorizontalGraphToShader(
            gaugePos.m_x,
            gaugePos.m_y,//位置
            gaugeSizeF, ratio, gaugeHandle
        );
    }

    SetUsePixelShader(-1);
    SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, ShaderRegister::glitch_buffer);
}
