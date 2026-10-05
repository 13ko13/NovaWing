#include "DxLib.h"
#include "ChargeExplosionCollider.h"
#include "Game/GameObjects/Bullet/ChargeBullet.h"

namespace
{
	//爆発範囲のダメージ
	constexpr int damage = 1000;
}


ChargeExplosionCollider::ChargeExplosionCollider(ChargeBullet& owner) :
	m_owner(owner)
{
}

ChargeExplosionCollider::~ChargeExplosionCollider()
{
}

std::vector<std::shared_ptr<ColliderShape>> ChargeExplosionCollider::GetCollision() const
{
	//爆発の当たり判定を取得
	return { m_owner.GetExplosionSphere() };
}

void ChargeExplosionCollider::OnCollision(const ICollider& other)
{

}

bool ChargeExplosionCollider::IsCollisionActive() const
{
	return m_owner.IsExplosionActive();
}

int ChargeExplosionCollider::GetAttackPower() const
{
	//雑魚はワンパン
	return damage;
}