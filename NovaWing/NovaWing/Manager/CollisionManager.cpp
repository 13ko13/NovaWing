#include "CollisionManager.h"
#include "Manager/BulletManager.h"
#include "Game/GameObjects/Bullet/PlayerBullet.h"
#include "Game/GameObjects/Bullet/ChargeBullet.h"
#include "Game/GameObjects/Bullet/EnemyBullet.h"
#include "Charactor/Enemy/FloatingEnemy/FloatingEnemy.h"
#include "Charactor/Player/Player.h"
#include "Charactor/Enemy/WormEnemy/WormEnemy.h"
#include "TargetManager.h"
#include "Game/GameObjects/Actors/Rock/Rock.h"
#include "Game/GameObjects/Camera/CameraBase.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/BossEnemy/BossEnemy.h"

namespace
{
	//ワームエネミー(頭・胴体共通)に接触した際のプレイヤーへのダメージ
	constexpr int worm_contact_damage = 40;
	//プレイヤーが岩に当たった時のダメージ
	constexpr int hit_rock_damage = 30;
	//プレイヤーがダメージを食らった時のカメラを揺らす力
	constexpr float shake_power = 7.0f;
	//プレイヤーがダメージを食らった時にどのくらいの時間カメラを揺らすか
	constexpr int shake_frame = 18;
}

CollisionManager::CollisionManager(
	const std::weak_ptr<Player> pPlayer,
	const std::weak_ptr<BulletManager> pBulletManager,
	const std::weak_ptr<CameraBase> pCamera,
	const std::weak_ptr<BossEnemy> pBoss) :
	m_pPlayer(pPlayer),
	m_pBulletManager(pBulletManager),
	m_pCamera(pCamera),
	m_pBoss(pBoss)
{

}

CollisionManager::~CollisionManager()
{

}

void CollisionManager::RegisterFloatingEnemy(std::shared_ptr<FloatingEnemy> pEnemy)
{
	//敵の配列に渡された敵を格納
	m_pFloatingEnemies.push_back(pEnemy);
}

void CollisionManager::RegisterWormEnemy(std::shared_ptr<WormEnemy> pEnemy)
{
	//敵の配列に渡された敵を格納
	m_pWormEnemies.push_back(pEnemy);
}

void CollisionManager::RegisterRock(std::shared_ptr<Rock> pRock)
{
	//渡された岩を配列として管理する
	m_pRocks.push_back(pRock);
}

void CollisionManager::Update()
{
	//shared_ptrに変換
	std::shared_ptr<BulletManager> pBulletManager = m_pBulletManager.lock();
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();
	std::shared_ptr<BossEnemy> pBoss = m_pBoss.lock();

	//プレイヤー弾の配列を取得
	std::vector<std::weak_ptr<PlayerBullet>> weakPlayerBullets = pBulletManager->GetPlayerBullets();
	std::vector<std::shared_ptr<PlayerBullet>> sharedPlayerBullet;

	//敵弾の配列を取得
	std::vector<std::weak_ptr<EnemyBullet>> weakEnemyBullets = pBulletManager->GetEnemyBullets();
	std::vector<std::shared_ptr<EnemyBullet>> sharedEnemyBullets;

	//チャージ弾の配列を取得
	std::vector<std::weak_ptr<ChargeBullet>> weakChargeBullets = pBulletManager->GetChargeBullets();
	std::vector<std::shared_ptr<ChargeBullet>> sharedChargeBullets;

	//カメラのshared_ptr化
	std::shared_ptr<CameraBase> sharedCamera = m_pCamera.lock();

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
	std::vector<std::shared_ptr<BulletBase>> sharedAllBullets;
	sharedAllBullets.insert(sharedAllBullets.end(), sharedPlayerBullet.begin(), sharedPlayerBullet.end());
	sharedAllBullets.insert(sharedAllBullets.end(), sharedChargeBullets.begin(), sharedChargeBullets.end());

	//敵も同様にshared_ptrに変換
	std::vector<std::shared_ptr<FloatingEnemy>> pSharedEnemies;//浮遊敵
	for (std::weak_ptr<FloatingEnemy> pWeakEnemy : m_pFloatingEnemies)
	{
		//shared_ptrに変換
		std::shared_ptr<FloatingEnemy> pSharedEnemy = pWeakEnemy.lock();
		//nullチェック
		if (!pSharedEnemy) continue;
		//変換したものを格納
		pSharedEnemies.push_back(pSharedEnemy);
	}
	std::vector<std::shared_ptr<WormEnemy>> pSharedWormEnemies;//ワームエネミー
	for (std::weak_ptr<WormEnemy> pWeakWormEnemy : m_pWormEnemies)
	{
		//shared_ptrに変換
		std::shared_ptr<WormEnemy> pSharedWormEnemy = pWeakWormEnemy.lock();
		//nullチェック
		if (!pSharedWormEnemy) continue;
		//変換したものを格納
		pSharedWormEnemies.push_back(pSharedWormEnemy);
	}

	//全ての浮遊敵とプレイヤーの弾が当たっているか
	for (std::shared_ptr<FloatingEnemy> pEnemy : pSharedEnemies)
	{
		//もしその敵が死んでいるなら処理をせずに次の敵の処理に移る
		if (pEnemy->IsDead()) continue;

		//全ての弾をループで見る
		for (std::shared_ptr<BulletBase> pPlayerBullet : sharedAllBullets)
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
			//カメラを揺らす
			sharedCamera->OnShake(shake_power,shake_frame);
		}
	}

	//プレイヤーの弾とワームエネミーの頭の
	//当たり判定が当たっているかを一つずつ調べる
	for (std::shared_ptr<WormEnemy> pWormEnemy : pSharedWormEnemies)
	{
		//もしその敵が死んでいるなら処理をせずに次の敵の処理に移る
		if (pWormEnemy->IsDead()) continue;
		//全ての弾をループで見る
		for (std::shared_ptr<BulletBase> pPlayerBullet : sharedAllBullets)
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

	//もしボスが死んでいるなら処理を行わない
	if (!pBoss->IsDead())
	{
		//ボスとプレイヤーの弾の当たり判定
		for (std::shared_ptr<BulletBase> pPlayerBullet : sharedAllBullets)
		{
			//ボスの球と弾の球が当たっていたら
			Sphere hitCol = pBoss->GetSphere();
			Sphere bulletCol = pPlayerBullet->GetSphere();
			if (hitCol.HitCollision(bulletCol))
			{
				//ボスの被弾処理
				pBoss->TakeDamage(pPlayerBullet->GetAttackPower());
				//敵に当たった時のプレイヤー弾の処理
				pPlayerBullet->OnHitEnemy();
			}
		}

		//ボスのビームとプレイヤーの当たり判定
		//ビームのすべての球を取得
		//std::vector<Sphere> spheresL = pBoss->
		//for()
	}

	//ワームの頭、胴体、どれか一つでも当たっていればtrueにする
	bool isHitWorm = false;

	//プレイヤーとワームの頭、胴体の当たり判定が当たっているかを一つずつ調べる
	//多重ヒットを起こさないために
	//プレイヤーがダメージをすでに食らっていれば処理を行わない

	for (std::shared_ptr<WormEnemy> pWormEnemy : pSharedWormEnemies)
	{
		//もしその敵が死んでいるなら処理をせずに次の敵の処理に移る
		if (pWormEnemy->IsDead()) continue;
		//プレイヤーの当たり判定を取得
		Sphere playerCol = pPlayer->GetSphere();
		//ワームエネミーの頭の当たり判定を取得
		Sphere wormHeadCol = pWormEnemy->GetHeadSphere();
		if (playerCol.HitCollision(wormHeadCol))
		{
			//当たったことを記録する
			isHitWorm = true;

			if (!pPlayer->IsTakingDamage())
			{
				//食らっていなければプレイヤーのHPを減らす
				pPlayer->TakeDamage(worm_contact_damage);
				//カメラを揺らす
				sharedCamera->OnShake(shake_power,shake_frame);
			}
		}
		//胴体の当たり判定を取得
		const std::vector<Sphere>& segmentSpheres = pWormEnemy->GetSegmentSpheres();
		for (const Sphere& segmentSphere : segmentSpheres)
		{
			if (playerCol.HitCollision(segmentSphere))
			{
				//当たったことを記録する
				isHitWorm = true;

				if (!pPlayer->IsTakingDamage())
				{
					//食らっていなければプレイヤーのHPを減らす
					pPlayer->TakeDamage(worm_contact_damage);
					//カメラを揺らす
					sharedCamera->OnShake(shake_power,shake_frame);
				}
			}
		}
	}

	std::vector<std::shared_ptr<Rock>> sharedRocks;
	for (std::weak_ptr<Rock> pWeakRock : m_pRocks)
	{
		//shared_ptrに変換
		std::shared_ptr<Rock> pSharedRock = pWeakRock.lock();
		//nullチェック
		if (!pSharedRock) continue;
		//変換したものを格納
		sharedRocks.push_back(pSharedRock);
	}

	//どれか一つでも岩が持っている球に当たっていれば記録する
	bool isHit = false;

	//岩とプレイヤーが当たっているかを1つずつ調べる
	for (std::shared_ptr<Rock> pRock : sharedRocks)
	{
		//プレイヤーの当たり判定を取得
		Sphere playerCol = pPlayer->GetSphere();
		//岩の当たり判定を取得
		std::vector<Sphere> rockCollisions = pRock->GetSpheres();

		//その岩の中のすべての球とプレイヤーの球が当たっているかを検出
		for (Sphere& rockColl : rockCollisions)
		{
			//当たっていて、ダメージを食らっていないときのみダメージを食らうようにする
			if (playerCol.HitCollision(rockColl))
			{
				//球と当たったことを記録
				isHit = true;

				if (!pPlayer->IsTakingDamage())
				{
					//一度当たったら離れるまで当たらないようにする
					//プレイヤーのHPを減らす
					pPlayer->TakeDamage(hit_rock_damage);
					//カメラを揺らす
					sharedCamera->OnShake(shake_power,shake_frame);
				}
			}
		}
	}

	//ワームの球と岩の球で
	//どの球にもあたっていない場合は
	if (!isHit && !isHitWorm)
	{
		//プレイヤーにダメージを食らっていないことを知らせる
		pPlayer->OnLeaveDamaging();
	}

	//死んでいるものは配列から消す
	m_pFloatingEnemies.erase(
	std::remove_if(m_pFloatingEnemies.begin(), m_pFloatingEnemies.end(),
		[](const std::weak_ptr<FloatingEnemy>& pEnemy)
		{
			return pEnemy.lock() == nullptr;
		}),
	m_pFloatingEnemies.end()
	);

	//死んでいるものは配列から消す
	m_pWormEnemies.erase(
	std::remove_if(m_pWormEnemies.begin(), m_pWormEnemies.end(),
		[](const std::weak_ptr<WormEnemy>& pEnemy)
		{
			return pEnemy.lock() == nullptr;
		}),
	m_pWormEnemies.end()
	);
}

