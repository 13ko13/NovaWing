#pragma once
#include "IBossEnemyState.h"

class Player;
class BossIdleState : public IBossEnemyState
{
public:
	BossIdleState(std::weak_ptr<BossEnemy> pBoss,
		std::weak_ptr<Player> pPlayer);
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

	//プレイヤーの弱参照
	std::weak_ptr<Player> m_pPlayer;
};

