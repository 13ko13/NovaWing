#pragma once
#include "Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/IGunshipBossState.h"

class GunshipBoss;
class ModelAnimator;
class GunshipBossRocketState : public IGunshipBossState
{
public:
	GunshipBossRocketState(std::weak_ptr<GunshipBoss> pBoss);
	virtual ~GunshipBossRocketState();

	void Enter() override;
	void Update() override;
	void Exit() override; 

private:
	//ロケットを撃つときの段階
	enum class RocketPhase
	{
		Open,//発射口が開く
		Close,//発射口が閉じる
	};
	RocketPhase m_rocketPhase = RocketPhase::Open;

	//アニメーター
	ModelAnimator& m_animator;
};
