#include "NoneState.h"

NoneState::NoneState(const std::weak_ptr<Player> pPlayer):
	ISpecialActionState(pPlayer)
{}

NoneState::~NoneState()
{}

void NoneState::Enter()
{}

void NoneState::Update()
{}

void NoneState::Exit()
{}
