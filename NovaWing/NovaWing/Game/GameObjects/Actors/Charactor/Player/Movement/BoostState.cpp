#include "BoostState.h"
#include "Charactor/Player/Player.h"
#include "Manager/SoundManager.h"

namespace
{
	constexpr float boost_speed = 17.0f;
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
}

void BoostState::Exit()
{
	//ゲージ使用を中止したことをプレイヤーに伝える
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();
	pPlayer->EndUseGauge();

	//ブースト音を止める
	m_pSoundManager.lock()->Stop(SoundManager::SoundType::Boost);
}

float BoostState::GetSpeed() const
{
	return boost_speed;
}

const char* BoostState::GetButtonName() const
{
	return "boost";
}