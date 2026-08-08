#include "EnemyFactory.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/FloatingEnemy/FloatingEnemy.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/WormEnemy/WormEnemy.h"
#include "Game/GameObjects/Actors/Charactor/Player/Player.h"
#include "Game/GameObjects/Camera/CameraBase.h"
#include "Manager/TargetManager.h"
#include "Manager/CollisionManager.h"

namespace
{
	//ワームの胴体の数
	constexpr int worm_segment_num = 5;
}

EnemyFactory::EnemyFactory(
	std::shared_ptr<Player> pPlayer,
		std::weak_ptr<BulletManager> pBulletManager,
		std::weak_ptr<CameraBase> pCamera,
		std::weak_ptr<TargetManager> pTargetManager,
		std::weak_ptr<CollisionManager> pCollisionManager) :
	m_pPlayer(pPlayer),
	m_pBulletManager(pBulletManager),
	m_pCamera(pCamera),
	m_pTargetManager(pTargetManager),
	m_pCollisionManager(pCollisionManager)
{
}

EnemyFactory::~EnemyFactory()
{
	//処理なし
}

std::shared_ptr<EnemyBase> EnemyFactory::Create(
	const Vector3& pos, EnemyType type)
{
	switch (type)
	{
		//エネミーの種類ごとに生成
	case EnemyType::FloatingEnemy://浮遊敵
	{
		std::shared_ptr<FloatingEnemy> pFloating = std::make_shared<FloatingEnemy>(
			m_pPlayer,
			ResourceLoader::ModelID::FloatingEnemy,
			m_pBulletManager,
			m_pCamera,
			pos
		);
		pFloating->Init();//初期化
		//ターゲットマネージャーと当たり判定マネージャーに登録
		if (m_pTargetManager.lock() != nullptr)
		{
			m_pTargetManager.lock()->RegisterFloatingEnemy(pFloating);
		}
		if (m_pCollisionManager.lock() != nullptr)
		{
			m_pCollisionManager.lock()->RegisterFloatingEnemy(pFloating);
		}

		return pFloating;
	}
	break;
	case EnemyType::WormEnemy://ワーム
	{
		//ワームに必要なデータ
		WormEnemy::WormEnemyData data;
		//動き出す位置は関係なくすぐ動き始める
		if (m_pPlayer.lock() != nullptr)
		{
			data.activatePlayerZ = m_pPlayer.lock()->GetPos().m_z;
		}
		else
		{
			data.activatePlayerZ = 0.0f;
		}
		data.direction = -1.0f;//手前に向かう
		data.modelID = ResourceLoader::ModelID::WormHead;
		data.pos = pos;
		data.segmentCount = worm_segment_num;

		std::shared_ptr<WormEnemy> pWorm = std::make_shared<WormEnemy>(
			m_pPlayer,
			m_pBulletManager,
			m_pCamera,
			data
		);
		pWorm->Init();//初期化
		//ターゲットマネージャーと当たり判定マネージャーに登録
		if (m_pTargetManager.lock() != nullptr)
		{
			m_pTargetManager.lock()->RegisterWormEnemy(pWorm);
		}
		if (m_pCollisionManager.lock() != nullptr)
		{
			m_pCollisionManager.lock()->RegisterWormEnemy(pWorm);
		}

		return pWorm;
	}
	break;

	default:
		return nullptr;
	}
	return nullptr;
}
