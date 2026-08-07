#include "SummonState.h"

SummonState::SummonState(std::weak_ptr<BossEnemy> pBoss,
		Position3 summonPos) :
	IBossEnemyState(pBoss),
	m_summonPos(summonPos)
{
}

SummonState::~SummonState()
{
}

void SummonState::Enter()
{
}

void SummonState::Update()
{
}

void SummonState::Exit()
{
}
