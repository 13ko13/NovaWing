#include "BoostState.h"
#include "Charactor/Player/Player.h"

namespace
{
	constexpr float boost_speed = 17.0f;
}

BoostState::BoostState(const std::weak_ptr<Player> pPlayer):
	GaugeActionStateBase(pPlayer)
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
}

void BoostState::Exit()
{
	//ゲージ使用を中止したことをプレイヤーに伝える
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();
	pPlayer->EndUseGauge();
}

float BoostState::GetSpeed() const
{
	return boost_speed;
}

const char* BoostState::GetButtonName() const
{
	return "boost";
}