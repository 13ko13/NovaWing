#include "DisabledMovementState.h"
#include "Game/GameObjects/Actors/Charactor/Player/Player.h"

DisabledMovementState::DisabledMovementState(const std::weak_ptr<Player> pPlayer):
	IMovementState(pPlayer)
{}

DisabledMovementState::~DisabledMovementState()
{}

void DisabledMovementState::Enter()
{
	//プレイヤーの速度を0にする
	Vector3 Vel = Vector3(0.0f, 0.0f, 0.0f);
	m_pPlayer.lock()->SetVel(Vel);
}

void DisabledMovementState::Update()
{}

void DisabledMovementState::Exit()
{}
