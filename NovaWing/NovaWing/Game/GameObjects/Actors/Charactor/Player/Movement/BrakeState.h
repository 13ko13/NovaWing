#pragma once
#include "GaugeActionStateBase.h"
class BrakeState :public GaugeActionStateBase
{
public:
	BrakeState(const std::weak_ptr<Player> pPlayer);
	~BrakeState();

	void Enter()override;//ステートに入った時
	void Exit() override;//ステートから出たとき

private:
	float GetSpeed() const override;//速度
	const char* GetButtonName() const override;//ボタン名
};

