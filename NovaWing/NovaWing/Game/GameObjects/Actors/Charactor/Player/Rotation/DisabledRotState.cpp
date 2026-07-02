#include "DisabledRotState.h"

DisabledRotState::DisabledRotState(const std::weak_ptr<Player> pPlayer) :
	IRotationState(pPlayer)
{}

DisabledRotState::~DisabledRotState()
{}

void DisabledRotState::Enter()
{}

void DisabledRotState::Update()
{}

void DisabledRotState::Exit()
{}
