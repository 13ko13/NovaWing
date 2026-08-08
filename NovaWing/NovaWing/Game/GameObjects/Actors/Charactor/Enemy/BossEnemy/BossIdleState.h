#pragma once
#include "IBossEnemyState.h"
class BossIdleState : public IBossEnemyState
{
public:
	BossIdleState(std::weak_ptr<BossEnemy> pBoss);
	~BossIdleState();

	void Enter() override;
	void Update() override;
	void Exit() override;

private:
	//攻撃フレーム更新
	int m_attackFrame = 0;

	//攻撃の種類
	enum class AttackType
	{
		Summon,//召喚
		Beam,//ビーム

		Max,//数
	};
};

