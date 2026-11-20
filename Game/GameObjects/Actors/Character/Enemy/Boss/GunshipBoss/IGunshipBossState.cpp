#include "IGunshipBossState.h"

IGunshipBossState::IGunshipBossState(std::weak_ptr<GunshipBoss> pEnemy) :
	m_pBoss(pEnemy)
{
}

IGunshipBossState::~IGunshipBossState()
{
}
