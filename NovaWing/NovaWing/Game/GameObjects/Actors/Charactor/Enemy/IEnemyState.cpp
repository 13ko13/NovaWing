#include "IEnemyState.h"

IEnemyState::IEnemyState(std::weak_ptr<Enemy> pEnemy):
	m_pEnemy(pEnemy)
{
}
