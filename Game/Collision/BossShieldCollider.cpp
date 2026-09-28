#include "BossShieldCollider.h"
#include "BulletCollider.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/BossEnemy/BossEnemy.h"

BossShieldCollider::BossShieldCollider(BossEnemy& owner) :
	m_owner(owner)
{
}

std::vector<std::shared_ptr<ColliderShape>> BossShieldCollider::GetCollision() const
{
	//球はボスのOnInitで作り直されるので、毎回ボスから取得する
	return { m_owner.GetInvinsibleSphere() };
}

void BossShieldCollider::OnCollision(const ICollider& other)
{
	if (other.GetTag() == ColliderTag::PlayerBullet)
	{
		const BulletCollider& bullet = static_cast<const BulletCollider&>(other);
		m_owner.OnHitInvincibleCol(bullet.GetPos(), bullet.GetAttackPower());
	}
}

bool BossShieldCollider::IsCollisionActive() const
{
	return !m_owner.IsDead();
}
