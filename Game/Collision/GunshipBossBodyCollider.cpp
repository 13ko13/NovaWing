#include "GunshipBossBodyCollider.h"
#include "Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.h"
#include "Game/Collision/CapsuleShape.h"
#include "Game/Collision/BulletCollider.h"

GunshipBossBodyCollider::GunshipBossBodyCollider(GunshipBoss& owner) :
	m_owner(owner)
{
}

GunshipBossBodyCollider::~GunshipBossBodyCollider()
{
}

std::vector<std::shared_ptr<ColliderShape>> GunshipBossBodyCollider::GetCollision() const
{
	//胴体のカプセルを返す
	return { m_owner.GetBodyCapsule() };
}

ColliderTag GunshipBossBodyCollider::GetTag() const
{
	//プレイヤーの弾・チャージショットとの組み合わせはCollisionManagerに登録済み
	return ColliderTag::BossDamage;
}

void GunshipBossBodyCollider::OnCollision(const ICollider& other)
{
	//プレイヤーの弾とチャージショットでダメージを受ける
	if (other.GetTag() == ColliderTag::PlayerBullet ||
		other.GetTag() == ColliderTag::ChargeBullet)
	{
		const BulletCollider& bullet = static_cast<const BulletCollider&>(other);
		//顔に当たったダメージを受ける
		m_owner.TakeBulletDamage(bullet.GetOwner(), false);
	}
}

bool GunshipBossBodyCollider::IsCollisionActive() const
{
	//ボスが生きていて、登場が終わっていたらtrueを返す(登場ムービー中は当たらない)
	return !m_owner.IsDead() && m_owner.IsBossAppear();
}
