#pragma once
#include "ISpiderBossState.h"
#include "Utility/Vector3.h"

class SpiderBoss;
class EnemyFactory;
class SpiderBossSummonState : public ISpiderBossState
{
public:
	SpiderBossSummonState(
		std::weak_ptr<SpiderBoss> pBoss,
		const Position3& summonPos,
		std::weak_ptr<EnemyFactory> enemyFactory
	);
	~SpiderBossSummonState();

	void Enter() override;
	void Update() override;
	void Exit() override;

private:
	//雑魚を召喚する場所
	Position3 m_summonPos;
	//敵生産工場
	std::weak_ptr<EnemyFactory> m_pEnemyFactory;

};

