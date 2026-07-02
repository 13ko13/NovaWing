#pragma once
#include "IMovementState.h"
#include <memory>

class Input;
class Player;
class IdleMovementState : public IMovementState
{
public:
	IdleMovementState(const std::weak_ptr<Player> pPlayer);
	~IdleMovementState();

	void Enter()override;//ステートに入った時
	void Update() override;//更新処理
	void Exit() override;//ステートから出たとき

private:
	
};

