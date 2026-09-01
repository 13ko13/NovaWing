#include "SpecialGaugeUI.h"
#include "Game/GameObjects/Actors/Charactor/Player/Player.h"
#include "Manager/ResourceLoader.h"
#include "Utility/SizeF.h"
#include "Constants/ShaderRegister.h"
#include "Utility/Size.h"
#include "Utility/GraphShaderDraw.h"
#include "Utility/Vector2.h"
#include "Main/Application.h"

namespace
{
    //ゲージ画像の位置
    const Vector2 hp_frame_pos_ratio = { 0.02f,0.11f };
    //スキャンラインを入れる周期
    constexpr float scanline_frequency = 655.0f;
}

SpecialGaugeUI::SpecialGaugeUI(std::weak_ptr<Player> pPlayer) :
    m_pPlayer(pPlayer)
{
    //スキャンラインを入れる周期をシェーダに渡す
    m_pCBuffGlitchData->scanlineFrequency = scanline_frequency;
    UpdateShaderConstantBuffer(m_cbufferGlitch);
}

SpecialGaugeUI::~SpecialGaugeUI()
{

}

void SpecialGaugeUI::Update()
{
    GaugeUIBase::Update();
}

void SpecialGaugeUI::Draw()
{
	//スペシャルゲージの枠画像を取得
	int gaugeFrameHandle = ResourceLoader::GetInstance().GetGraphic(
		ResourceLoader::GraphicID::SpecialGaugeFrame
	);
	//スペシャルゲージの画像を取得
	int gaugeHandle = ResourceLoader::GetInstance().GetGraphic(
		ResourceLoader::GraphicID::SpecialGauge
	);

	const Size& wsize = Application::GetInstance().GetWindowSize();
	Vector2 frameDrawPos = Vector2(
		wsize.m_width * hp_frame_pos_ratio.m_x,
		wsize.m_height * hp_frame_pos_ratio.m_y
	);

	//HPの割合から、切り取り位置を計算
	float ratio =
		static_cast<float>(m_pPlayer.lock()->GetSpecialGauge()) /
		static_cast<float>(m_pPlayer.lock()->GetMaxSpecialGauge());

	//ゲージを描画
	DrawGauge(
		gaugeFrameHandle,
		gaugeHandle,
		frameDrawPos,
		ratio);

#ifdef _DEBUG
	
#endif
}