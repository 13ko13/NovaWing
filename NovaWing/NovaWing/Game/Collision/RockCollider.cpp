#include "RockCollider.h"
#include "../GameObjects/Actors/Rock/Rock.h"

RockCollider::RockCollider(Rock& owner) :
    m_owner(owner)
{

}

RockCollider::~RockCollider()
{
}

std::vector<std::shared_ptr<ColliderShape>> RockCollider::GetCollision() const
{
    return std::vector<std::shared_ptr<ColliderShape>>(m_spheres.begin(), m_spheres.end());
}

ColliderTag RockCollider::GetTag() const 
{
    return ColliderTag::Rock;
}

void RockCollider::OnCollision(const ICollider& other)
{

}

bool RockCollider::IsCollisionActive() const
{
    return true;
}

void RockCollider::AddSphere(const Vector3& pos, float radius)
{
    m_spheres.push_back(std::make_shared<SphereShape>(pos, radius));
}
