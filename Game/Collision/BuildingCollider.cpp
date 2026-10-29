#include "BuildingCollider.h"

#include "../GameObjects/Actors/Building/Building.h"
#include "MeshShape.h"

BuildingCollider::BuildingCollider(Building& owner, int colModelHandle) :
	m_owner(owner)
{
	//当たり判定用のメッシュを生成
	m_mesh = std::make_shared<MeshShape>(colModelHandle);
}

BuildingCollider::~BuildingCollider()
{
}

std::vector<std::shared_ptr<ColliderShape>> BuildingCollider::GetCollision() const
{
	return { m_mesh };
}

bool BuildingCollider::IsCollisionActive() const
{
	return true;
}

void BuildingCollider::OnCollision(const ICollider& other)
{
}

int BuildingCollider::GetOwnerID() const
{
	return m_owner.GetID();
}
