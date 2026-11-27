#include "GunshipBossWeakCollider.h"
#include "Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.h"
#include "Game/Collision/SphereShape.h"
#include "Game/Collision/BulletCollider.h"

GunshipBossWeakCollider::GunshipBossWeakCollider(GunshipBoss& owner) :
	m_owner(owner)
{
}

GunshipBossWeakCollider::~GunshipBossWeakCollider()
{
}

std::vector<std::shared_ptr<ColliderShape>> GunshipBossWeakCollider::GetCollision() const
{
	//左右の弱点の球を返す
	return { m_owner.GetWeakSphereLeft(), m_owner.GetWeakSphereRight() };
}

ColliderTag GunshipBossWeakCollider::GetTag() const
{
	return ColliderTag::BossDamage;
}

void GunshipBossWeakCollider::OnCollision(const ICollider& other)
{
	//プレイヤーの弾とチャージショットでダメージを受ける
	if (other.GetTag() == ColliderTag::PlayerBullet ||
		other.GetTag() == ColliderTag::ChargeBullet)
	{
		const BulletCollider& bullet = static_cast<const BulletCollider&>(other);
		//ポッドに当たったダメージを受ける
		m_owner.TakeBulletDamage(bullet.GetOwner(), true);
	}
}

bool GunshipBossWeakCollider::IsCollisionActive() const
{
	//ボスが生きていて、登場が終わっていたらtrueを返す
	return !m_owner.IsDead() && m_owner.IsBossAppear();
}
