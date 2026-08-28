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
	//浮遊敵のHP
	constexpr int floating_enemy_hp = 5;
}

EnemyFactory::EnemyFactory(
	std::shared_ptr<Player> pPlayer,
		std::weak_ptr<BulletManager> pBulletManager,
		std::weak_ptr<CameraBase> pCamera,
		std::weak_ptr<TargetManager> pTargetManager,
		std::weak_ptr<CollisionManager> pCollisionManager,
		std::weak_ptr<SoundManager> pSoundManager) :
	m_pPlayer(pPlayer),
	m_pBulletManager(pBulletManager),
	m_pCamera(pCamera),
	m_pTargetManager(pTargetManager),
	m_pCollisionManager(pCollisionManager),
	m_pSoundManager(pSoundManager)
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
			pos,
			floating_enemy_hp,
			m_pSoundManager
		);
		pFloating->Init();//初期化
		//ターゲットマネージャーと当たり判定マネージャーに登録
		if (m_pTargetManager.lock() != nullptr)
		{
			m_pTargetManager.lock()->Register(pFloating);
		}
		if (m_pCollisionManager.lock() != nullptr)
		{
			m_pCollisionManager.lock()->Register(pFloating);
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
			data,
			m_pSoundManager
		);
		pWorm->Init();//初期化
		//ターゲットマネージャーと当たり判定マネージャーに登録
		if (m_pTargetManager.lock() != nullptr)
		{
			m_pTargetManager.lock()->Register(pWorm);
		}
		if (m_pCollisionManager.lock() != nullptr)
		{
			m_pCollisionManager.lock()->Register(pWorm);
		}

		return pWorm;
	}
	break;

	default:
		return nullptr;
	}
	return nullptr;
}
