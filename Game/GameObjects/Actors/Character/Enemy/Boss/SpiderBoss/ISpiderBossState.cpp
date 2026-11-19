#include "ISpiderBossState.h"

ISpiderBossState::ISpiderBossState(std::weak_ptr<SpiderBoss> pEnemy) :
	m_pBoss(pEnemy)
{
}
