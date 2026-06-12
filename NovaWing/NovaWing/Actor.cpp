#include "Actor.h"

namespace
{
	constexpr int max_health = 100;
}

Actor::Actor() :
	m_position(0.0f, 0.0f, 0.0f),
	m_rotation(),
	m_velocity(0.0f, 0.0f, 0.0f),
	m_health(max_health)
{
}

Actor::~Actor()
{
	//実装なし
}

void Actor::Rotate(const Vector3& axis, float angle)
{
	//クォータニオンを作ってm_rotationに掛け算(複合回転)
	Quaternion q = Quaternion(axis, angle);
	m_rotation = m_rotation * q;
}

bool Actor::IsDead() const
{
	return m_health <= 0;
}
