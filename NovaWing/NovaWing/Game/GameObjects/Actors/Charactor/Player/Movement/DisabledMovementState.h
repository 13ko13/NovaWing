#pragma once
#include "IMovementState.h"
#include <memory>
class DisabledMovementState : public IMovementState
{
public:
	DisabledMovementState(const std::weak_ptr<Player> pPlayer);
	~DisabledMovementState();

	void Enter()override;//ステートに入った時
	void Update() override;//更新処理
	void Exit() override;//ステートから出たとき

private:
};

