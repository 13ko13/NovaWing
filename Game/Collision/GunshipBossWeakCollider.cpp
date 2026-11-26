#include "GunshipBossWeakCollider.h"
#include "Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.h"
#include "Game/Collision/SphereShape.h"
#include "Game/Collision/BulletCollider.h"

namespace
{
	//放熱中のポッドに当たったときのダメージ倍率
	constexpr float weak_open_damage_rate = 2.0f;
	//閉じているポッドに当たったときのダメージ倍率
	constexpr float weak_closed_damage_rate = 1.0f / 15.0f;
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
		//ポッドが閉じているか開いているかでダメージ倍率を変える
		float damageRate = weak_closed_damage_rate;
		if (m_owner.IsWeakOpen())
		{
			damageRate = weak_open_damage_rate;
		}

		//小数のダメージも使うためにfloatでためておく
		m_storedDamage += static_cast<float>(bullet.GetAttackPower()) * damageRate;
		//ためたダメージの整数部分だけボスに与える
		int damage = static_cast<int>(m_storedDamage);
		//与えた分を引いて、小数部分は次に持ち越す
		m_storedDamage -= static_cast<float>(damage);
		m_owner.TakeDamage(damage);
	}
}

bool GunshipBossWeakCollider::IsCollisionActive() const
{
	//ボスが生きていて、登場が終わっていたらtrueを返す
	return !m_owner.IsDead() && m_owner.IsBossAppear();
}
