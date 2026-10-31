#include "EnemyCollider.h"
#include "BulletCollider.h"
#include "Game/GameObjects/Actors/Character/Enemy/EnemyBase.h"
#include "Game/Collision/ChargeExplosionCollider.h"

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
		//倒されたときに何で倒されたか分かるように、ダメージより先に記録する
		m_owner.SetLastDamageSource(EnemyBase::DamageSource::NormalShot);
		m_owner.TakeDamage(bullet.GetAttackPower());
	}
	//タグが爆発だった場合は火力が高くなる
	else if (other.GetTag() == ColliderTag::ChargeExplosion)
	{
		const ChargeExplosionCollider& explosion =
			static_cast<const ChargeExplosionCollider&>(other);
		//倒されたときに何で倒されたか分かるように、ダメージより先に記録する
		m_owner.SetLastDamageSource(EnemyBase::DamageSource::ChargeShot);
		m_owner.TakeDamage(explosion.GetAttackPower());
	}
	else if(other.GetTag() == ColliderTag::Player)
	{
		m_owner.OnHitPlayer();
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
