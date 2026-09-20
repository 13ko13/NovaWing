#include "PlayerCollider.h"
#include "Game/GameObjects/Actors/Charactor/Player/Player.h"

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
}

bool PlayerCollider::IsCollisionActive() const
{
	return !m_owner.IsDead();
}

void PlayerCollider::UpdateShape(const Vector3& pos, float radius)
{
	m_sphere->Update(pos, radius);
}
