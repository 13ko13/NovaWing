#include "IBossEnemyState.h"

IBossEnemyState::IBossEnemyState(std::weak_ptr<BossEnemy> pEnemy) :
	m_pBoss(pEnemy)
{
}
