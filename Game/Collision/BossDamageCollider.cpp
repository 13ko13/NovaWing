#include "BossDamageCollider.h"
#include "BulletCollider.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/BossEnemy/BossEnemy.h"

BossDamageCollider::BossDamageCollider(BossEnemy& owner) :
	m_owner(owner)
{
}

std::vector<std::shared_ptr<ColliderShape>> BossDamageCollider::GetCollision() const
{
	//球はボスのOnInitで作り直されるので、毎回ボスから取得する
	return { m_owner.GetDamageSphere() };
}

void BossDamageCollider::OnCollision(const ICollider& other)
{
	if (other.GetTag() == ColliderTag::PlayerBullet)
	{
		const BulletCollider& bullet = static_cast<const BulletCollider&>(other);
		m_owner.TakeDamage(bullet.GetAttackPower());
	}
}

bool BossDamageCollider::IsCollisionActive() const
{
	return !m_owner.IsDead();
}
