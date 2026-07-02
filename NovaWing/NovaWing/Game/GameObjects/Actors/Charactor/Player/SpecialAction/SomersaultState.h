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
	//宙返りを行うための現在のフレーム数
	int m_frame = 0.0f;
};