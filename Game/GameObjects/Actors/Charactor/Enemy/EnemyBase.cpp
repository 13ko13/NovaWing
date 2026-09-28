#include "EnemyBase.h"
#include "Game/GameObjects/Actors/Charactor/Player/Player.h"

EnemyBase::EnemyBase(
	ResourceLoader::ModelID modelID,
	std::weak_ptr<CameraBase> pCamera,
	std::weak_ptr<Player> pPlayer,
	std::weak_ptr<BulletManager> pBulletManager,
	int maxHealth):
	Charactor(modelID,pCamera,maxHealth),
	m_pPlayer(pPlayer),
	m_pBulletManager(pBulletManager)
{
}

EnemyBase::~EnemyBase()
{
}

void EnemyBase::OnEnemyDead()
{
	//プレイヤーに敵が死んだことを伝える
	if (m_pPlayer.lock() != nullptr)
	{
		m_pPlayer.lock()->AddDefeatedEnemyCount();	
	}

	OnDead();
}
