#include "GunshipBossWeakCollider.h"
#include "Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBossNetState.h"
#include "Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.h"
#include "Game/Collision/SphereShape.h"
#include "Game/Collision/BulletCollider.h"

namespace
{
	//弱点に当たったときのダメージ倍率(胴体は1倍)
	constexpr int weak_damage_rate = 2;
}

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
		m_owner.TakeDamage(bullet.GetAttackPower() * weak_damage_rate);
	}
}

bool GunshipBossWeakCollider::IsCollisionActive() const
{
	//ボスが生きていて、今のステートが網攻撃のステートかつ
	//弱点が出ていたらtrueを返す
	std::shared_ptr<GunshipBossNetState> netState =
		std::dynamic_pointer_cast<GunshipBossNetState>(m_owner.GetCurrentState());

	return !m_owner.IsDead() && netState != nullptr && netState->IsVentOpen();
}
