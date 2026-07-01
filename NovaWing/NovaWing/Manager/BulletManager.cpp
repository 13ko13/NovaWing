#include "BulletManager.h"
#include"Game/GameObjects/Actors/Bullet/PlayerBullet.h"
#include "Game/GameObjects/Actors/Bullet/EnemyBullet.h"

BulletManager::BulletManager()
{
}

BulletManager::~BulletManager()
{
}

void BulletManager::CreateBullet(const BulletType bulletType, const Vector3& pos,
	const Vector3& vel, const int attackPower)
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
		break;
	}
	}
}

std::vector<std::shared_ptr<PlayerBullet>> BulletManager::GetPlayerBullets() const
{
	return 
}

std::vector<std::shared_ptr<EnemyBullet>> BulletManager::GetEnemyBullets() const
{
	return std::vector<std::shared_ptr<EnemyBullet>>();
}
