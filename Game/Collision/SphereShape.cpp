#include "SphereShape.h"

namespace
{
	constexpr int div_num = 8;
}

SphereShape::SphereShape():
	ColliderShape(Shape::Sphere),
	m_pos(Vector3()),
	m_radius(0.0f)
{
	
}
SphereShape::SphereShape(const Vector3& pos,float radius):
	ColliderShape(Shape::Sphere),
	m_pos(pos),
	m_radius(radius)
{
	
}

void SphereShape::Update(const Vector3& pos, float radius)
{
	m_pos = pos;
	m_radius = radius;
}

void SphereShape::Draw(unsigned int color, bool isFill)
{
	DrawSphere3D(m_pos.ToDxLib(), m_radius, div_num, color, color, isFill);
}

bool SphereShape::HitCollision(const SphereShape& other) const
{
	//球同士の中点の距離を求める
	Vector3 centerToCenter = other.m_pos - m_pos;
	//球同士の半径を足した値
	float radiusSum = m_radius + other.m_radius;
	//中点の距離が半径の和より小さければあたっている
	return centerToCenter.Length() < radiusSum;
}