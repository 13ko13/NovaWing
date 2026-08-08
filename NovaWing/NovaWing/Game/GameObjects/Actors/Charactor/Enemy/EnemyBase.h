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
		std::weak_ptr<BulletManager> pBulletManager
	);
	virtual ~EnemyBase();

private:

protected:
	//プレイヤーの弱参照
	std::weak_ptr<Player> m_pPlayer;
	//バレットマネージャーの弱参照
	std::weak_ptr<BulletManager> m_pBulletManager;
	//死んでるか
	bool m_isDying = false;
};

