#include "BulletManager.h"
#include"Game/GameObjects/Bullet/PlayerBullet.h"
#include "Game/GameObjects/Bullet/EnemyBullet.h"
#include "Game/GameObjects/Bullet/ChargeBullet.h"
#include "Game/GameObjects/GameObject.h"


BulletManager::BulletManager()
{
}

BulletManager::~BulletManager()
{
}

void BulletManager::Update()
{
	//死んだ弾は削除する
	m_pAllBullets.erase(
		std::remove_if(
			m_pAllBullets.begin(),
			m_pAllBullets.end(),
			[](const std::weak_ptr<BulletBase>& pBullet)
			{
				if(pBullet.lock() != nullptr) 
				{
					return pBullet.lock()->IsDead();
				}
				return true;
			}),
			m_pAllBullets.end()
		);
}

void BulletManager::CreateBullet(const BulletType bulletType, const Vector3& pos,
	const Vector3& vel, const int attackPower,
		std::weak_ptr<GameObject> pTarget)
{
	switch (bulletType)
	{
	case BulletType::PlayerBullet://プレイヤーの弾
	{
		//インスタンスを作成
		std::shared_ptr<PlayerBullet> pBullet =
			std::make_shared<PlayerBullet>(pos, vel, attackPower);

		//初期化
		pBullet->Init();

		//それを配列に格納する
		m_pPlayerBullets.push_back(pBullet);
		m_pAllBullets.push_back(pBullet);

		break;
	}
	case BulletType::EnemyBullet://敵の弾
	{
		//敵の弾も同様に作成
		//インスタンスを作成
		std::shared_ptr<EnemyBullet> pBullet =
			std::make_shared<EnemyBullet>(pos, vel, attackPower);

		//初期化
		pBullet->Init();

		//それを配列に格納する
		m_pEnemyBullets.push_back(pBullet);
		m_pAllBullets.push_back(pBullet);

		break;
	}
	case BulletType::ChargeBullet://プレイヤーのチャージ弾
	{
		//チャージ弾はターゲットを受け取る必要がある
		std::shared_ptr<ChargeBullet> pBullet =
			std::make_shared<ChargeBullet>(pos, vel, attackPower, pTarget);

		///初期化
		pBullet->Init();

		//それを配列に格納する
		m_pChargeBullets.push_back(pBullet);
		m_pAllBullets.push_back(pBullet);

		break;
	}
	}
}
