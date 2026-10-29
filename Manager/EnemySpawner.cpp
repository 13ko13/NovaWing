#include "EnemySpawner.h"

EnemySpawner::EnemySpawner(
	std::weak_ptr<Player> pPlayer,
	std::weak_ptr<EnemyFactory> pEnemyFactory) :
	m_pPlayer(pPlayer),
	m_pEnemyFactory(pEnemyFactory)
{
}

EnemySpawner::~EnemySpawner()
{
}
