#include "BulletManager.h"
#include"Game/GameObjects/Actors/Bullet/PlayerBullet.h"

BulletManager::BulletManager()
{
}

BulletManager::~BulletManager()
{
}

void BulletManager::CreateBullet(const BulletType bulletType, const Vector3& pos,
	const Vector3& vel, const int attackPower, ResourceLoader::ModelID modelId)
{
	switch (bulletType)
	{
	case BulletType::PlayerBullet://プレイヤーの弾
	{
		//インスタンスを作成
		std::shared_ptr<PlayerBullet> pBullet =
			std::make_shared<PlayerBullet>(pos, vel, attackPower, modelId);

		//初期化
		pBullet->Init();

		//それを配列に格納する
		m_pBullets.push_back(pBullet);

		break;
		//TODO:エネミーの弾も同様に作成
	}
	}
}
