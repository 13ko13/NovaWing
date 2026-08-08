#pragma once
#include <memory>
#include <vector>

class Player;
class BulletManager;
class FloatingEnemy;
class WormEnemy;
class Rock;
class CameraBase;
class CollisionManager
{
public:
	CollisionManager(const std::weak_ptr<Player> pPlayer,
		const std::weak_ptr<BulletManager> pBulletManager,
		const std::weak_ptr<CameraBase> pCamera
	);
	~CollisionManager();

	//敵を配列に格納する
	void RegisterFloatingEnemy(std::shared_ptr<FloatingEnemy> pEnemy);
	void RegisterWormEnemy(std::shared_ptr<WormEnemy> pEnemy);
	void RegisterRock(std::shared_ptr<Rock> pRock);
	void Update();//更新処理

private:
	//プレイヤー
	std::weak_ptr<Player> m_pPlayer;
	//カメラ
	std::weak_ptr<CameraBase> m_pCamera;
	//弾のマネージャー
	std::weak_ptr<BulletManager> m_pBulletManager;
	//浮遊敵の配列
	std::vector<std::weak_ptr<FloatingEnemy>> m_pFloatingEnemies;
	//ワームエネミーの配列
	std::vector<std::weak_ptr<WormEnemy>> m_pWormEnemies;
	//岩の配列
	std::vector<std::weak_ptr<Rock>> m_pRocks;
};

