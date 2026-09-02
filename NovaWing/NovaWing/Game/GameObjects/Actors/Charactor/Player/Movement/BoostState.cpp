#include <EffekseerForDXLib.h>

#include "BoostState.h"
#include "Charactor/Player/Player.h"
#include "Manager/SoundManager.h"

namespace
{
	constexpr float boost_speed = 17.0f;
	//ブースト音がフェードアウトする時間
	constexpr float boost_fade_out_time = 60.0f;

	//ブーストエフェクトのオフセット位置
	const Vector3 boost_effect_offset_pos = Vector3(0.0f, 0.0f, -200.0f);
}

BoostState::BoostState(const std::weak_ptr<Player> pPlayer,
	std::weak_ptr<SoundManager> pSoundManager):
	GaugeActionStateBase(pPlayer, pSoundManager)
{

}

BoostState::~BoostState()
{

}

void BoostState::Enter()
{
	//ゲージ使用を開始したことをプレイヤーに伝える
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();
	pPlayer->StartUseGauge();

	//ブースト音を鳴らす
	m_pSoundManager.lock()->Play(SoundManager::SoundType::Boost);

	//ブーストエフェクトを出す
	int boostEffectH = ResourceLoader::GetInstance().GetEffect(ResourceLoader::EffectID::Boost);
	m_boostPlayEffect = PlayEffekseer3DEffect(boostEffectH);
}

void BoostState::Update()
{
	//基底クラスの移動・ゲージ処理を呼ぶ
	GaugeActionStateBase::Update();

	//プレイヤー
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();

	//プレイヤーより少し後ろの位置にエフェクトを出す
	VECTOR effectPos = (
		pPlayer->GetPos() + pPlayer->GetVisualForward() * 
		boost_effect_offset_pos.m_z).ToDxLib();

	//プレイヤーから回転角を取得してエフェクトの角度を変える
	SetRotationPlayingEffekseer3DEffect(
		m_boostPlayEffect,
		pPlayer->GetRotationX(),
		pPlayer->GetRotationY() + DX_PI_F,
		0.0f
	);

	//エフェクトの位置
	SetPosPlayingEffekseer3DEffect(
		m_boostPlayEffect,
		effectPos.x,
		effectPos.y,
		effectPos.z
	);
}

void BoostState::Exit()
{
	//ゲージ使用を中止したことをプレイヤーに伝える
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();
	pPlayer->EndUseGauge();

	//ブースト音を止める
	m_pSoundManager.lock()->FadeOut(SoundManager::SoundType::Boost, boost_fade_out_time);
	//エフェクトも止める
	StopEffekseer3DEffect(m_boostPlayEffect);
}

float BoostState::GetSpeed() const
{
	return boost_speed;
}

const char* BoostState::GetButtonName() const
{
	return "boost";
}