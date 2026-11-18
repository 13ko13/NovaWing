#include "SpiderBossBeamCollider.h"
#include "Game/GameObjects/Actors/Character/Enemy/SpiderBoss/SpiderBoss.h"
#include "Game/GameObjects/Actors/Character/Enemy/SpiderBoss/SpiderBossBeamState.h"

SpiderBossBeamCollider::SpiderBossBeamCollider(SpiderBoss& owner) :
	m_owner(owner)
{
}

std::vector<std::shared_ptr<ColliderShape>> SpiderBossBeamCollider::GetCollision() const
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

void SpiderBossBeamCollider::OnCollision(const ICollider& other)
{
}

bool SpiderBossBeamCollider::IsCollisionActive() const
{
	if (m_owner.IsDead()) return false;
	return std::dynamic_pointer_cast<SpiderBossBeamState>(m_owner.GetCurrentState()) != nullptr;
}

int SpiderBossBeamCollider::GetDamage() const
{
	return std::dynamic_pointer_cast<SpiderBossBeamState>(m_owner.GetCurrentState())->GetBeamDamage();
}

int SpiderBossBeamCollider::GetOwnerID() const
{
	return m_owner.GetID();
}
