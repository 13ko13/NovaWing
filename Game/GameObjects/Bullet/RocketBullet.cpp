#include "RocketBullet.h"

RocketBullet::RocketBullet(const RocketBulletData& data) :
	EnemyBullet(data.pos, data.vel, data.attackPower,
	data.pCamera, data.pShooter, data.pEffectManager),
	m_speed(data.vel.Length())
{
}

RocketBullet::~RocketBullet()
{
}
