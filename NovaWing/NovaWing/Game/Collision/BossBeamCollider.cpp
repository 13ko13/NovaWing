#include "BossBeamCollider.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/BossEnemy/BossEnemy.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/BossEnemy/BossBeamState.h"

BossBeamCollider::BossBeamCollider(BossEnemy& owner) :
	m_owner(owner)
{
}

std::vector<std::shared_ptr<ColliderShape>> BossBeamCollider::GetCollision() const
{
	//GetBeamSphereL/Rはビームステート中しか呼べないので、IsCollisionActiveを確認してから呼ぶこと
	std::vector<std::shared_ptr<ColliderShape>> shapes;
	for (const std::shared_ptr<SphereShape>& sphere : m_owner.GetBeamSphereL())
	{
		shapes.push_back(sphere);
	}
	for (const std::shared_ptr<SphereShape>& sphere : m_owner.GetBeamSphereR())
	{
		shapes.push_back(sphere);
	}
	return shapes;
}

void BossBeamCollider::OnCollision(const ICollider& other)
{
}

bool BossBeamCollider::IsCollisionActive() const
{
	if (m_owner.IsDead()) return false;
	return std::dynamic_pointer_cast<BossBeamState>(m_owner.GetCurrentState()) != nullptr;
}

int BossBeamCollider::GetDamage() const
{
	return std::dynamic_pointer_cast<BossBeamState>(m_owner.GetCurrentState())->GetBeamDamage();
}

int BossBeamCollider::GetOwnerID() const
{
	return m_owner.GetID();
}
