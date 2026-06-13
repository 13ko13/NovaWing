#include "NormalState.h"

NormalState::NormalState(std::shared_ptr<InputManager> pInputManager,
	std::shared_ptr<Player> pPlayer) :
	IPlayerState(pPlayer),
	m_pInputManager(pInputManager)
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
