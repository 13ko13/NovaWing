#include "BoostState.h"

BoostState::BoostState(const std::weak_ptr<Player> pPlayer):
	IMovementState(pPlayer)
{

}

BoostState::~BoostState()
{

}

void BoostState::Enter()
{
	//処理なし
}

void BoostState::Update()
{

}

void BoostState::Exit()
{

}
