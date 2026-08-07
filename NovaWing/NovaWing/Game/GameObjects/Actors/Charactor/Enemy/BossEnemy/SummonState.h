#pragma once
#include "IBossEnemyState.h"
#include "Utility/Vector3.h"

class BossEnemy;
class SummonState : public IBossEnemyState
{
public:
	SummonState(std::weak_ptr<BossEnemy> pBoss,
		Position3 summonPos);
	~SummonState();

	void Enter() override;
	void Update() override;
	void Exit() override;

private:
	//雑魚を召喚する場所
	Position3 m_summonPos;
};

