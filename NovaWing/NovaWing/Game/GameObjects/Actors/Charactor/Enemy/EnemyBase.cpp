#include "EnemyBase.h"

EnemyBase::EnemyBase(
	ResourceLoader::ModelID modelID,
	std::weak_ptr<CameraBase> pCamera,
	std::weak_ptr<Player> pPlayer,
	std::weak_ptr<BulletManager> pBulletManager):
	Charactor(modelID,pCamera),
	m_pPlayer(pPlayer),
	m_pBulletManager(pBulletManager)
{
}

EnemyBase::~EnemyBase()
{
}
