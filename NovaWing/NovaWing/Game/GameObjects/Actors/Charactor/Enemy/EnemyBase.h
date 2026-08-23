#pragma once
#include <memory>
#include "Game/GameObjects/Actors/Charactor/Charactor.h"

class Player;
class BulletManager;
class CameraBase;
class EnemyBase : public Charactor
{
public:
	EnemyBase(
		ResourceLoader::ModelID modelID,
		std::weak_ptr<CameraBase> pCamera,
		std::weak_ptr<Player> pPlayer,
		std::weak_ptr<BulletManager> pBulletManager,
		int maxHealth = 100
	);
	virtual ~EnemyBase();

	//死亡待機状態かを返す
	bool IsDying() const { return m_isDying; }

	//死んだときに呼ぶ関数
	void OnEnemyDead();

private:

protected:
	//プレイヤーの弱参照
	std::weak_ptr<Player> m_pPlayer;
	//バレットマネージャーの弱参照
	std::weak_ptr<BulletManager> m_pBulletManager;
	//死亡待機状態か
	bool m_isDying = false;
};

