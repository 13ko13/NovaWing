#include <DxLib.h>
#include <cassert>

#include "PlayerHPGaugeUI.h"
#include "Game/GameObjects/Actors/Charactor/Player/Player.h"
#include "Manager/ResourceLoader.h"
#include "Utility/SizeF.h"
#include "Constants/ShaderRegister.h"
#include "Utility/Size.h"
#include "Utility/GraphShaderDraw.h"
#include "Utility/Vector2.h"
#include "Manager/InputManager.h"

namespace
{
	//スキャンラインを入れる周期
	constexpr float scanline_frequency = 74.0f;
	const Vector2 hp_frame_pos = { 30.0f,30.0f };//HP枠画像の位置
}

PlayerHPGaugeUI::PlayerHPGaugeUI(std::weak_ptr<Player> pPlayer) :
	HPGaugeUIBase(pPlayer)
{
	//スキャンラインを入れる周期をシェーダに渡す
	m_pCBuffGlitchData->scanlineFrequency = scanline_frequency;
	UpdateShaderConstantBuffer(m_cbufferGlitch);
}

PlayerHPGaugeUI::~PlayerHPGaugeUI()
{
}

void PlayerHPGaugeUI::Update()
{
	HPGaugeUIBase::Update();

#ifdef _DEBUG
	//デバッグのため、スキャンラインを入れる周期を上げ下げできるようにしておく
	if (InputManager::GetInstance().IsPressed(
		InputEvent::upScanlineFrequency))
	{
		m_scanlineFrequency++;
	}
	if (InputManager::GetInstance().IsPressed(
		InputEvent::downScanlineFrequency))
	{
		m_scanlineFrequency--;
	}
	/*m_pCBuffGlitchData->scanlineFrequency = m_scanlineFrequency;
	UpdateShaderConstantBuffer(m_cbufferGlitch);*/
#endif
}

void PlayerHPGaugeUI::Draw()
{
	//HPの枠画像を取得
	int hpFrameHandle = ResourceLoader::GetInstance().GetGraphic(
		ResourceLoader::GraphicID::PlayerHPFrame
	);
	//HPゲージの画像を取得
	int hpGaugeHandle = ResourceLoader::GetInstance().GetGraphic(
		ResourceLoader::GraphicID::PlayerHPGauge
	);

	DrawHPGauge(hpFrameHandle, hpGaugeHandle, hp_frame_pos);

#ifdef _DEBUG
	DrawFormatString(0, 115, 0xffffff, L"scanlineFrequency : %f", m_scanlineFrequency);
#endif
}
