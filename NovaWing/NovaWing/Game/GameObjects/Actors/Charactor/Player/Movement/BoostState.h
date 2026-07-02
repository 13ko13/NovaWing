#pragma once
#include "GaugeActionStateBase.h"
class BoostState :public GaugeActionStateBase
{
public:
	BoostState(const std::weak_ptr<Player> pPlayer);
	~BoostState();

	void Enter()override;//ステートに入った時
	void Exit() override;//ステートから出たとき

private:
	float GetSpeed() const override;//速度
	const char* GetButtonName() const override;//ボタン名
};

