#pragma once
#include "IRotationState.h"
class DisabledRotState : public IRotationState
{
public:
	DisabledRotState(const std::weak_ptr<Player> pPlayer);
	~DisabledRotState();

	void Enter()override;//ステートに入った時
	void Update() override;//更新処理
	void Exit() override;//ステートから出たとき

private:
};

