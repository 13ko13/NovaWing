#include "BulletCollider.h"
#include "Game/GameObjects/Bullet/BulletBase.h"

BulletCollider::BulletCollider(BulletBase& owner, ColliderTag tag) :
	m_owner(owner),
	m_tag(tag)
{
}

std::vector<std::shared_ptr<ColliderShape>> BulletCollider::GetCollision() const
{
	return { m_owner.GetSphere() };
}

void BulletCollider::OnCollision(const ICollider& other)
{
	//何に当たっても弾は消える
	m_owner.OnHitEnemy();
}

bool BulletCollider::IsCollisionActive() const
{
	return !m_owner.IsDead();
}

int BulletCollider::GetAttackPower() const
{
	return m_owner.GetAttackPower();
}

Vector3 BulletCollider::GetPos() const
{
	return m_owner.GetPos();
}
