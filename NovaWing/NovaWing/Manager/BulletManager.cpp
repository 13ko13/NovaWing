#include "BulletManager.h"
#include"Game/GameObjects/Actors/Bullet/PlayerBullet.h"

BulletManager::BulletManager()
{
}

BulletManager::~BulletManager()
{
}

void BulletManager::Update()
{
	//全ての弾の更新処理を呼ぶ
	for (std::shared_ptr<BulletBase> bullet : m_pBullets)
	{
		bullet->Update();
	}

	//もし死亡判定の弾があれば配列から削除する
	//消すべき要素を末尾に集める
	auto it = std::remove_if(
		m_pBullets.begin(),
		m_pBullets.end(),
		[](std::shared_ptr<BulletBase> pBullet)
		{
			return pBullet->IsDead();
		}
	);
	//集めた最後の要素から一番最後の要素までを削除
	m_pBullets.erase(it,m_pBullets.end());
}

void BulletManager::Draw()
{
	//全ての弾の描画処理を呼ぶ
	for (std::shared_ptr<BulletBase> bullet : m_pBullets)
	{
		bullet->Draw();
	}
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
