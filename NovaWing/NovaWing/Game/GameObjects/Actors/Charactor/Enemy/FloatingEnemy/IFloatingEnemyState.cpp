#include "IFloatingEnemyState.h"

IFloatingEnemyState::IFloatingEnemyState(std::weak_ptr<FloatingEnemy> pEnemy):
	m_pEnemy(pEnemy)
{
}
