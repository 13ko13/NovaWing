#pragma once
#include "ISpecialActionState.h"

class SomersaultState : public ISpecialActionState
{
public:
	SomersaultState(const std::weak_ptr<Player> pPlayer);
	~SomersaultState();

	void Enter()override;//ステートに入った時
	void Update() override;//更新処理
	void Exit() override;//ステートから出たとき

private:

};