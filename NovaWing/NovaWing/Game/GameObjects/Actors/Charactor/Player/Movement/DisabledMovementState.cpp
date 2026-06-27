#include "DisabledMovementState.h"

DisabledMovementState::DisabledMovementState(const std::weak_ptr<Player> pPlayer):
	IMovementState(pPlayer)
{}

DisabledMovementState::~DisabledMovementState()
{}

void DisabledMovementState::Enter()
{}

void DisabledMovementState::Update()
{}

void DisabledMovementState::Exit()
{}
