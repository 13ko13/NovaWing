#include "CollisionManager.h"
#include "Manager/BulletManager.h"
#include "Game/GameObjects/Actors/Bullet/PlayerBullet.h"
#include "Game/GameObjects/Actors/Bullet/EnemyBullet.h"
#include "Charactor/Enemy/FloatingEnemy.h"
#include "Charactor/Player/Player.h"

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
	std::vector<std::weak_ptr<PlayerBullet>> weakPlayerBullets = pBulletManager->GetPlayerBullets();
	std::vector<std::shared_ptr<PlayerBullet>> sharedPlayerBullet;

	//敵弾の配列を取得
	std::vector<std::weak_ptr<EnemyBullet>> weakEnemyBullets = pBulletManager->GetEnemyBullets();
	std::vector<std::shared_ptr<EnemyBullet>> sharedEnemyBullets;

	//弾のすべてのweak_ptrをshared_ptrに変換
	for (std::weak_ptr<PlayerBullet>& weakBullet : weakPlayerBullets)//プレイヤー弾
	{
		std::shared_ptr<PlayerBullet> pBullet = weakBullet.lock();

		//nullチェック
		if (!pBullet) continue;

		//変換したものを格納
		sharedPlayerBullet.push_back(pBullet);
	}
	for (std::weak_ptr<EnemyBullet>& weakBullet : weakEnemyBullets)//敵弾
	{
		std::shared_ptr<EnemyBullet> pBullet = weakBullet.lock();

		//nullチェック
		if (!pBullet) continue;

		//変換したものを格納
		sharedEnemyBullets.push_back(pBullet);
	}

	//敵も同様にshared_ptrに変換
	std::vector<std::shared_ptr<FloatingEnemy>> pSharedEnemies;
	for (std::weak_ptr<FloatingEnemy> pWeakEnemy : m_pEnemies)
	{
		//shared_ptrに変換
		std::shared_ptr<FloatingEnemy> pSharedEnemy = pWeakEnemy.lock();
		//nullチェック
		if (!pSharedEnemy) continue;
		//変換したものを格納
		pSharedEnemies.push_back(pSharedEnemy);
	}

	//全ての敵と弾が当たっているか
	for (std::shared_ptr<FloatingEnemy> pEnemy : pSharedEnemies)
	{
		//もしその敵が死んでいるなら処理をせずに次の敵の処理に移る
		if (pEnemy->IsDead()) continue;

		//全ての弾をループで見る
		for (std::shared_ptr<PlayerBullet> pPlayerBullet : sharedPlayerBullet)
		{
			Sphere enemyCol = pEnemy->GetSphere();//敵の球
			Sphere bulletCol = pPlayerBullet->GetSphere();

			//当たっていたら
			if (enemyCol.HitCollision(bulletCol))
			{
				//弾の攻撃力分、敵のHPを減らす
				pEnemy->TakeDamage(pPlayerBullet->GetAttackPower());
				//敵に当たったときのプレイヤー弾の処理
				pPlayerBullet->OnHitEnemy();
			}
		}
	}

	//プレイヤーとすべての敵の弾が当たっているかを一つずつ調べる
	for (std::shared_ptr<EnemyBullet> pEnemyBullet : sharedEnemyBullets)
	{
		//死亡済みの弾は処理せずに、次の弾の処理に移る
		if (pEnemyBullet->IsDead()) continue;

		//プレイヤーの当たり判定を取得
		Sphere playerCol = pPlayer->GetSphere();
		//敵弾の当たり判定を取得
		Sphere enemyBulletCol = pEnemyBullet->GetSphere();
		//当たっていたら
		if (playerCol.HitCollision(enemyBulletCol))
		{
			//プレイヤーのHPを減らす
			pPlayer->TakeDamage(pEnemyBullet->GetAttackPower());
			//敵弾の当たったときの処理
			pEnemyBullet->OnHitPlayer();
		}
	}
}

