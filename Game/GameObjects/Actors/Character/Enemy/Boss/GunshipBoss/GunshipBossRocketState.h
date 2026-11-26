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
	//発射口を開ける時の更新処理
	void UpdateRocketOpen();
	//ロケット発射時の更新処理
	void UpdateRocketFire();
	//発射口を閉じるときの更新処理
	void UpdateRocketClose();

	//ロケット発射
	void FireRocket();

private:
	//ロケットを撃つときの段階
	enum class RocketPhase
	{
		Open,//発射口が開く
		Fire,//発射
		Close,//発射口が閉じる
	};
	RocketPhase m_rocketPhase = RocketPhase::Open;

	//アニメーター
	ModelAnimator& m_animator;

	//ロケット発射の間隔を数える変数
	int m_rocketFrame = 0;
	//ロケットを発射した数
	int m_rocketFiredNum = 0;
};
