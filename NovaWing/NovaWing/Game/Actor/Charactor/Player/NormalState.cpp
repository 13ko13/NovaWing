#include "NormalState.h"

NormalState::NormalState(std::shared_ptr<Player> pPlayer) :
	IPlayerState(pPlayer)
{
}

NormalState::~NormalState()
{
	//処理なし
}

void NormalState::Enter()
{
}

void NormalState::Update()
{
	
}

void NormalState::Exit()
{
}
