#pragma once
#include "Game/GameObjects/Bullet/EnemyBullet.h"

class RocketBullet : public EnemyBullet
{
public:
	//RocketBulletに必要な引数
	struct RocketBulletData
	{
		Vector3 pos;
		Vector3 vel;
		int attackPower;
		std::weak_ptr<CameraBase> pCamera;
		std::shared_ptr<EnemyBase> pShooter;
		std::weak_ptr<EffectManager> pEffectManager;
	};

	RocketBullet(const RocketBulletData& data);
	virtual ~RocketBullet();

private:
	//弾の速さ
	float m_speed = 0.0f;
};
