#pragma once
#include "GaugeActionStateBase.h"
class SoundManager;
class BoostState :public GaugeActionStateBase
{
public:
	BoostState(const std::weak_ptr<Player> pPlayer,
		std::weak_ptr<SoundManager> pSoundManager);
	~BoostState();

	void Enter()override;//ステートに入った時
	void Update() override;//更新
	void Exit() override;//ステートから出たとき

private:
	float GetSpeed() const override;//速度
	const char* GetButtonName() const override;//ボタン名

	//ブーストエフェクト
	int m_boostPlayEffect = -1;
};

