#pragma once
#include "IMovementState.h"
class BoostState : public IMovementState
{
public:
	BoostState(const std::weak_ptr<Player> pPlayer);
	~BoostState();

	void Enter()override;//ステートに入った時
	void Update() override;//更新処理
	void Exit() override;//ステートから出たとき

private:
};

