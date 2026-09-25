#pragma once
#include <memory>
#include <vector>
#include <set>

#include "Game/GameObjects/Actors/Charactor/DamageSource.h"

class Player;
class BulletManager;
class EnemyBase;
class Rock;
class GameCamera;
class BossEnemy;
class ICollider;
class CollisionManager
{
public:
	CollisionManager(const std::weak_ptr<Player> pPlayer,
		const std::weak_ptr<BulletManager> pBulletManager,
		const std::weak_ptr<GameCamera> pCamera,
		const std::weak_ptr<BossEnemy> pBoss
	);
	~CollisionManager();

	//敵を配列に格納する
	void Register(std::shared_ptr<EnemyBase> pEnemy);
	void RegisterRock(std::shared_ptr<Rock> pRock);
	void Update();//更新処理

private:
	//2つのコライダーが当たった時の処理
	void OnHit(ICollider& a, ICollider& b);

private:
	//プレイヤー
	std::weak_ptr<Player> m_pPlayer;
	//カメラ
	std::weak_ptr<GameCamera> m_pCamera;
	//弾のマネージャー
	std::weak_ptr<BulletManager> m_pBulletManager;
	//敵の配列
	std::vector<std::weak_ptr<EnemyBase>> m_pEnemies;
	//岩の配列
	std::vector<std::weak_ptr<Rock>> m_pRocks;
	//ボス
	std::weak_ptr<BossEnemy> m_pBoss;

	//多段ヒット防止用、今フレーム当たっていたダメージ源
	std::set<DamageSource> m_hitSourcesThisFrame;
	//多段ヒット防止用、前フレーム当たっていたダメージ源
	std::set<DamageSource> m_hitSourcesPrevFrame;
};
