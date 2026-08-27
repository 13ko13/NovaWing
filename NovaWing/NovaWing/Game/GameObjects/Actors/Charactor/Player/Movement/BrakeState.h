#pragma once
#include "GaugeActionStateBase.h"
class SoundManager;
class BrakeState :public GaugeActionStateBase
{
public:
	BrakeState(const std::weak_ptr<Player> pPlayer,
		std::weak_ptr<SoundManager> pSoundManager);
	~BrakeState();

	void Enter()override;//ステートに入った時
	void Exit() override;//ステートから出たとき

private:
	float GetSpeed() const override;//速度
	const char* GetButtonName() const override;//ボタン名
};

