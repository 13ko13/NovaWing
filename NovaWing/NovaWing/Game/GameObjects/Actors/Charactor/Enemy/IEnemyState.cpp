#include "IEnemyState.h"

IEnemyState::IEnemyState(std::weak_ptr<FloatingEnemy> pEnemy):
	m_pEnemy(pEnemy)
{
}
