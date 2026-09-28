#include "PlayerCollider.h"
#include "BulletCollider.h"
#include "BossBeamCollider.h"
#include "Game/GameObjects/Actors/Charactor/Player/Player.h"

namespace
{
	//ワームエネミー(頭・胴体共通)に接触した際のプレイヤーへのダメージ
	constexpr int worm_contact_damage = 20;
	//プレイヤーが岩に当たった時のダメージ
	constexpr int hit_rock_damage = 20;
}

PlayerCollider::PlayerCollider(Player& owner):
	m_owner(owner),
	m_sphere(std::make_shared<SphereShape>())
{
}

std::vector<std::shared_ptr<ColliderShape>> PlayerCollider::GetCollision() const
{
	return { m_sphere };
}

void PlayerCollider::OnCollision(const ICollider& other)
{
	//タグで相手の具体的な型が決まるのでstatic_castしてよい
	switch (other.GetTag())
	{
	case ColliderTag::EnemyBullet:
		m_owner.TakeDamage(static_cast<const BulletCollider&>(other).GetAttackPower());
		break;
	case ColliderTag::BossBeam:
		m_owner.TakeDamage(static_cast<const BossBeamCollider&>(other).GetDamage());
		break;
	case ColliderTag::Worm:
		m_owner.TakeDamage(worm_contact_damage);
		break;
	case ColliderTag::Rock:
		m_owner.TakeDamage(hit_rock_damage);
		break;
	default:
		break;
	}
}

bool PlayerCollider::IsCollisionActive() const
{
	return !m_owner.IsDead();
}

void PlayerCollider::UpdateShape(const Vector3& pos, float radius)
{
	m_sphere->Update(pos, radius);
}
