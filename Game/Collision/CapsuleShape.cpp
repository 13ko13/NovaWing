#include "CapsuleShape.h"
#include "SphereShape.h"

namespace
{
	//デバッグ描画の分割数
	constexpr int div_num = 8;
}

CapsuleShape::CapsuleShape(const Vector3& startPos, const Vector3& endPos, float radius) :
	ColliderShape(Shape::Capsule),
	m_startPos(startPos),
	m_endPos(endPos),
	m_radius(radius)
{
}

CapsuleShape::~CapsuleShape()
{
}

void CapsuleShape::Update(const Vector3& startPos, const Vector3& endPos, float radius)
{
	m_startPos = startPos;
	m_endPos = endPos;
	m_radius = radius;
}

void CapsuleShape::Draw(unsigned int color, bool isFill)
{
	DrawCapsule3D(m_startPos.ToDxLib(), m_endPos.ToDxLib(), m_radius, div_num, color, color, isFill);
}

bool CapsuleShape::HitCollision(const SphereShape& sphere) const
{
	//球の中心と線分の距離が、半径の和より小さければ当たっている
	return HitCheck_Sphere_Capsule(
		sphere.GetPos().ToDxLib(), sphere.GetRadius(),
		m_startPos.ToDxLib(), m_endPos.ToDxLib(), m_radius);
}
