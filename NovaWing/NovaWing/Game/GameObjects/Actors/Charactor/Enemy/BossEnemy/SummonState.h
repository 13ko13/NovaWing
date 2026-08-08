#pragma once
#include "IBossEnemyState.h"
#include "Utility/Vector3.h"

class BossEnemy;
class EnemyFactory;
class SummonState : public IBossEnemyState
{
public:
	SummonState(
		std::weak_ptr<BossEnemy> pBoss,
		const Position3& summonPos,
		std::weak_ptr<EnemyFactory> enemyFactory
	);
	~SummonState();

	void Enter() override;
	void Update() override;
	void Exit() override;

private:
	//雑魚を召喚する場所
	Position3 m_summonPos;
	//敵生産工場
	std::weak_ptr<EnemyFactory> m_pEnemyFactory;
};

