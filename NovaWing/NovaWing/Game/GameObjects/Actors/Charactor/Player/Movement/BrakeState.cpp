#include "BrakeState.h"
#include "Charactor/Player/Player.h"

namespace
{
	constexpr float brake_speed = 2.0f;
}

BrakeState::BrakeState(const std::weak_ptr<Player> pPlayer) :
	GaugeActionStateBase(pPlayer)
{

}

BrakeState::~BrakeState()
{

}

void BrakeState::Enter()
{
	//ゲージ使用を開始したことをプレイヤーに伝える
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();
	pPlayer->StartUseGauge();
}

void BrakeState::Exit()
{
	//ゲージ使用を中止したことをプレイヤーに伝える
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();
	pPlayer->EndUseGauge();
}

float BrakeState::GetSpeed() const
{
	return brake_speed;
}

const char* BrakeState::GetButtonName() const
{
	return "brake";
}
