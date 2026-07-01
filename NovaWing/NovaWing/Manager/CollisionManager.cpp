#include "CollisionManager.h"
#include "Manager/BulletManager.h"
#include "Game/GameObjects/Actors/Bullet/PlayerBullet.h"
#include "Charactor/Enemy/FloatingEnemy.h"

CollisionManager::CollisionManager(const std::weak_ptr<Player> pPlayer, 
	const std::weak_ptr<BulletManager> pBulletManager):
	m_pPlayer(pPlayer),
	m_pBulletManager(pBulletManager)
{

}

CollisionManager::~CollisionManager()
{

}

void CollisionManager::RegisterEnemy(std::shared_ptr<FloatingEnemy> pEnemy)
{
	//敵の配列に渡された敵を格納
	m_pEnemies.push_back(pEnemy);
}

void CollisionManager::Update()
{
	//shared_ptrに変換
	std::shared_ptr<BulletManager> pBulletManager = m_pBulletManager.lock();
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();

	//プレイヤー弾の配列を取得
	std::vector<std::weak_ptr<PlayerBullet>> weakBullets = pBulletManager->GetPlayerBullets();
	std::vector<std::shared_ptr<PlayerBullet>> sharedBullet;
	//弾のすべてのweak_ptrをshared_ptrに変換
	for (std::weak_ptr<PlayerBullet>& weakBullet : weakBullets)
	{
		std::shared_ptr<PlayerBullet> pBullet = weakBullet.lock();
		//変換したものを格納
		sharedBullet.push_back(pBullet);
	}
	//敵も同様にshared_ptrに変換
	std::vector<std::shared_ptr<FloatingEnemy>> pSharedEnemies;
	for (std::weak_ptr<FloatingEnemy> pWeakEnemy : m_pEnemies)
	{
		//shared_ptrに変換
		std::shared_ptr<FloatingEnemy> pSharedEnemy = pWeakEnemy.lock();
		//変換したものを格納
		pSharedEnemies.push_back(pSharedEnemy);
	}

	//全ての敵と弾が当たっているか
	for (std::shared_ptr<FloatingEnemy> pEnemy : pSharedEnemies)
	{
		//もしその敵が死んでいるなら処理をしない
		if (pEnemy->IsDead()) return;

		//全ての弾をループで見る
		for (std::shared_ptr<PlayerBullet> pPlayerBullet : sharedBullet)
		{
			Sphere enemyCol = pEnemy->GetSphere();//敵の球
			Sphere bulletCol = pPlayerBullet->GetSphere();

			//当たっていたら
			if (enemyCol.HitCollision(bulletCol))
			{

			}
		}
	}
}

