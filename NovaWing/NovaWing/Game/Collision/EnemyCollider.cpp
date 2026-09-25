#include "EnemyCollider.h"
#include "BulletCollider.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/EnemyBase.h"

EnemyCollider::EnemyCollider(EnemyBase& owner, ColliderTag tag) :
	m_owner(owner),
	m_tag(tag)
{
}

std::vector<std::shared_ptr<ColliderShape>> EnemyCollider::GetCollision() const
{
	std::vector<std::shared_ptr<SphereShape>> spheres = m_owner.GetCollisionSpheres();
	return std::vector<std::shared_ptr<ColliderShape>>(spheres.begin(), spheres.end());
}

void EnemyCollider::OnCollision(const ICollider& other)
{
	if (other.GetTag() == ColliderTag::PlayerBullet)
	{
		//タグがPlayerBulletなら相手はBulletCollider
		const BulletCollider& bullet = static_cast<const BulletCollider&>(other);
		m_owner.TakeDamage(bullet.GetAttackPower());
	}
}

bool EnemyCollider::IsCollisionActive() const
{
	return !m_owner.IsDead();
}

int EnemyCollider::GetOwnerID() const
{
	return m_owner.GetID();
}
