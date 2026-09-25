#include "CounterCollider.h"
#include "Game/GameObjects/Actors/Charactor/Player/Player.h"

CounterCollider::CounterCollider(Player& owner):
	m_owner(owner)
{
	m_sphere = std::make_shared<SphereShape>();
}

CounterCollider::~CounterCollider()
{
}

std::vector<std::shared_ptr<ColliderShape>> CounterCollider::GetCollision() const
{
	return { m_sphere };
}

void CounterCollider::OnCollision(const ICollider& other)
{

}

void CounterCollider::UpdateShape(const Vector3& pos, float radius)
{
	m_sphere->Update(pos, radius);
}

bool CounterCollider::IsCollisionActive() const
{
	//ローリング中のみtrueを返す
	return m_owner.IsRolling();
}