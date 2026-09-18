#pragma once
#include "ColliderShape.h"

class SphereShape : public ColliderShape
{
public:
	SphereShape(float radius);

	//半径を取得
	float GetRadius() const { return m_radius; }
	//半径を設定
	void SetRadius(float radius) { m_radius = radius; }

private:
	float m_radius;//半径
};

