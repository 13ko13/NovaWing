#pragma once
#include "IRotationState.h"
class DefaultRotationState : public IRotationState
{
public:
	DefaultRotationState(const std::weak_ptr<Player> pPlayer);
	~DefaultRotationState();

	void Enter()override;//ステートに入った時
	void Update() override;//更新処理
	void Exit() override;//ステートから出たとき

private:

};