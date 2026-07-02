#pragma once
#include "ISpecialActionState.h"
class NoneState : public ISpecialActionState
{
public:
	NoneState(const std::weak_ptr<Player> pPlayer);
	~NoneState();

	void Enter()override;//ステートに入った時
	void Update() override;//更新処理
	void Exit() override;//ステートから出たとき

private:
};

