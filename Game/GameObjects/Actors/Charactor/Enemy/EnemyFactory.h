#pragma once
#include <memory>
#include "Manager/ResourceLoader.h"

class Player;
class BulletManager;
class CameraBase;
struct Vector3;
class EnemyBase;
class TargetManager;
class CollisionManager;
class SoundManager;
class EnemyFactory
{
public:
	EnemyFactory(std::shared_ptr<Player> pPlayer,
		std::weak_ptr<BulletManager> pBulletManager,
		std::weak_ptr<CameraBase> pCamera,
		std::weak_ptr<TargetManager> pTargetManager,
		std::weak_ptr<CollisionManager> pCollisionManager,
		std::weak_ptr<SoundManager> pSoundManager
	);

	~EnemyFactory();

	enum class EnemyType
	{
		FloatingEnemy,//浮遊敵
		WormEnemy,//ワームエネミー

		Max,//数
	};
	
	std::shared_ptr<EnemyBase> Create(
		const Vector3& pos,
		EnemyType type
	);

private:
	//プレイヤー
	std::weak_ptr<Player> m_pPlayer;
	std::weak_ptr<BulletManager> m_pBulletManager;
	std::weak_ptr<CameraBase> m_pCamera;
	std::weak_ptr<TargetManager> m_pTargetManager;
	std::weak_ptr<CollisionManager> m_pCollisionManager;
	std::weak_ptr<SoundManager> m_pSoundManager;
};