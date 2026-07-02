#pragma once
#include <memory>
#include <vector>

class Player;
class BulletManager;
class FloatingEnemy;
class CollisionManager
{
public:
	CollisionManager(const std::weak_ptr<Player> pPlayer,
		const std::weak_ptr<BulletManager> pBulletManager);
	~CollisionManager();

	//敵を配列に格納する
	void RegisterEnemy(std::shared_ptr<FloatingEnemy> pEnemy);
	void Update();//更新処理

private:
	//プレイヤー
	std::weak_ptr<Player> m_pPlayer;
	//弾のマネージャー
	std::weak_ptr<BulletManager> m_pBulletManager;
	//敵の配列
	std::vector<std::weak_ptr<FloatingEnemy>> m_pEnemies;
};

