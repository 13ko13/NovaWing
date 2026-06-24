#include "Sphere.h"

Sphere::Sphere(const Vector3& pos):
	m_pos(pos)
{

}

Sphere::~Sphere()
{
}

void Sphere::Update(const Vector3& pos, const float radius)
{
	m_pos = pos;
	m_radius = radius;
}

void Sphere::Draw(unsigned int color)
{
	DrawSphere3D(m_pos.ToDxLib(), m_radius, 8, color, color, FALSE);
}

bool Sphere::HitCollision(const Sphere& other) const
{
	//あたっているか
	bool isHit;

	//球同士の中点の距離を求める
	Vector3 centerToCenter = other.m_pos - m_pos;
	//球同士の半径を足した値
	float radiusSum = m_radius + other.m_radius;
	//中点の距離が半径の和より小さい場合はあたっている
	if (centerToCenter.Length() < radiusSum)
	{
		isHit = true;
	}
	else
	{
		//大きい場合はあたっていない
		isHit = false;
	}

	return isHit;
}
