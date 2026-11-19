#include "SpiderBossDamageCollider.h"
#include "BulletCollider.h"
#include "Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBoss.h"

namespace
{
	//反射されたビームを食らった時のダメージ
	constexpr int reflect_beam_damage = 200;
}

SpiderBossDamageCollider::SpiderBossDamageCollider(SpiderBoss& owner) :
	m_owner(owner)
{
}

std::vector<std::shared_ptr<ColliderShape>> SpiderBossDamageCollider::GetCollision() const
{
	//球はボスのOnInitで作り直されるので、毎回ボスから取得する
	return { m_owner.GetDamageSphere() };
}

void SpiderBossDamageCollider::OnCollision(const ICollider& other)
{
	if (other.GetTag() == ColliderTag::PlayerBullet)
	{
		const BulletCollider& bullet = static_cast<const BulletCollider&>(other);
		m_owner.TakeDamage(bullet.GetAttackPower());
	}
	else if (other.GetTag() == ColliderTag::BossBeamReflect)
	{
		m_owner.TakeDamage(reflect_beam_damage);
	}
	else if (other.GetTag() == ColliderTag::ChargeBullet)
	{
		//ダメージを受ける
		const BulletCollider& bullet = static_cast<const BulletCollider&>(other);
		m_owner.TakeDamage(bullet.GetAttackPower());
	}
}

bool SpiderBossDamageCollider::IsCollisionActive() const
{
	return !m_owner.IsDead();
}
