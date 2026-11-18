#include "BossBase.h"

BossBase::BossBase(BossBaseData& data) :
	EnemyBase(data.modelID,
	data.pCamera,
	data.pPlayer,
	data.pBulletManager,
	data.pSoundManager,
	data.pEffectManager)
{
}

BossBase::~BossBase()
{
}
