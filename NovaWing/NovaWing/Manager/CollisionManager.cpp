#include "CollisionManager.h"
#include "Manager/BulletManager.h"
#include "Game/GameObjects/Bullet/PlayerBullet.h"
#include "Game/GameObjects/Bullet/ChargeBullet.h"
#include "Game/GameObjects/Bullet/EnemyBullet.h"
#include "Charactor/Enemy/FloatingEnemy/FloatingEnemy.h"
#include "Charactor/Player/Player.h"
#include "Charactor/Enemy/WormEnemy/WormEnemy.h"
#include "TargetManager.h"

namespace
{
	//ワームエネミー(頭・胴体共通)に接触した際のプレイヤーへのダメージ
	constexpr int worm_contact_damage = 1;
}

CollisionManager::CollisionManager(const std::weak_ptr<Player> pPlayer, 
	const std::weak_ptr<BulletManager> pBulletManager):
	m_pPlayer(pPlayer),
	m_pBulletManager(pBulletManager)
{

}

CollisionManager::~CollisionManager()
{

}

void CollisionManager::RegisterFloatingEnemy(std::shared_ptr<FloatingEnemy> pEnemy)
{
	//敵の配列に渡された敵を格納
	m_pEnemies.push_back(pEnemy);
}

void CollisionManager::RegisterWormEnemy(std::shared_ptr<WormEnemy> pEnemy)
{
	//敵の配列に渡された敵を格納
	m_pWormEnemies.push_back(pEnemy);
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

	//チャージ弾の配列を取得
	std::vector<std::weak_ptr<ChargeBullet>> weakChargeBullets = pBulletManager->GetChargeBullets();
	std::vector<std::shared_ptr<ChargeBullet>> sharedChargeBullets;

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
	for (std::weak_ptr<ChargeBullet>& weakBullet : weakChargeBullets)//チャージ弾
	{
		std::shared_ptr<ChargeBullet> pBullet = weakBullet.lock();

		//nullチェック
		if (!pBullet) continue;

		//変換したものを格納
		sharedChargeBullets.push_back(pBullet);
	}

	//チャージ弾とノーマル弾を一つにまとめる
	std::vector<std::shared_ptr<BulletBase>> sharedBullets;
	sharedBullets.insert(sharedBullets.end(),sharedPlayerBullet.begin(),sharedPlayerBullet.end());
	sharedBullets.insert(sharedBullets.end(),sharedChargeBullets.begin(),sharedChargeBullets.end());

	//敵も同様にshared_ptrに変換
	std::vector<std::shared_ptr<FloatingEnemy>> pSharedEnemies;//浮遊敵
	for (std::weak_ptr<FloatingEnemy> pWeakEnemy : m_pEnemies)
	{
		//shared_ptrに変換
		std::shared_ptr<FloatingEnemy> pSharedEnemy = pWeakEnemy.lock();
		//nullチェック
		if (!pSharedEnemy) continue;
		//変換したものを格納
		pSharedEnemies.push_back(pSharedEnemy);
	}
	std::vector<std::shared_ptr<WormEnemy>> pSharedWormEnemies;//ワームエネミー
	for(std::weak_ptr<WormEnemy> pWeakWormEnemy : m_pWormEnemies)
	{
		//shared_ptrに変換
		std::shared_ptr<WormEnemy> pSharedWormEnemy = pWeakWormEnemy.lock();
		//nullチェック
		if(!pSharedWormEnemy) continue;
		//変換したものを格納
		pSharedWormEnemies.push_back(pSharedWormEnemy);
	}

	//全ての浮遊敵と弾が当たっているか
	for (std::shared_ptr<FloatingEnemy> pEnemy : pSharedEnemies)
	{
		//もしその敵が死んでいるなら処理をせずに次の敵の処理に移る
		if (pEnemy->IsDead()) continue;

		//全ての弾をループで見る
		for (std::shared_ptr<BulletBase> pPlayerBullet : sharedBullets)
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
			pEnemyBullet->OnHitEnemy();
		}
	}

	//プレイヤーの弾とワームエネミーの頭の
	//当たり判定が当たっているかを一つずつ調べる
	for (std::shared_ptr<WormEnemy> pWormEnemy : pSharedWormEnemies)
	{
		//もしその敵が死んでいるなら処理をせずに次の敵の処理に移る
		if (pWormEnemy->IsDead()) continue;
		//全ての弾をループで見る
		for (std::shared_ptr<BulletBase> pPlayerBullet : sharedBullets)
		{
			Sphere wormHeadCol = pWormEnemy->GetHeadSphere();//ワームエネミーの頭の球
			Sphere bulletCol = pPlayerBullet->GetSphere();
			//当たっていたら
			if (wormHeadCol.HitCollision(bulletCol))
			{
				//弾の攻撃力分、敵のHPを減らす
				pWormEnemy->TakeDamage(pPlayerBullet->GetAttackPower());
				//敵に当たったときのプレイヤー弾の処理
				pPlayerBullet->OnHitEnemy();
			}
		}
	}

	//プレイヤーとワームの頭、胴体の当たり判定が当たっているかを一つずつ調べる
	for(std::shared_ptr<WormEnemy> pWormEnemy : pSharedWormEnemies)
	{
		//もしその敵が死んでいるなら処理をせずに次の敵の処理に移る
		if(pWormEnemy->IsDead()) continue;
		//プレイヤーの当たり判定を取得
		Sphere playerCol = pPlayer->GetSphere();
		//ワームエネミーの頭の当たり判定を取得
		Sphere wormHeadCol = pWormEnemy->GetHeadSphere();
		if(playerCol.HitCollision(wormHeadCol))
		{
			//プレイヤーのHPを減らす
			pPlayer->TakeDamage(worm_contact_damage);
		}
		//胴体の当たり判定を取得
		const std::vector<Sphere>& segmentSpheres = pWormEnemy->GetSegmentSpheres();
		for(const Sphere& segmentSphere : segmentSpheres)
		{
			if(playerCol.HitCollision(segmentSphere))
			{
				//プレイヤーのHPを減らす
				pPlayer->TakeDamage(worm_contact_damage);
				break;
			}
		}
	}
}

