#include "PlayerCollider.h"

PlayerCollider::PlayerCollider(Player& owner)
{
}

std::shared_ptr<ColliderShape> PlayerCollider::GetCollision() const
{
	return std::shared_ptr<ColliderShape>();
}

void PlayerCollider::OnCollision(const ICollider& other)
{
}

bool PlayerCollider::IsCollisionActive() const
{
	return false;
}
