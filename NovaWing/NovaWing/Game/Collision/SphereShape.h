#pragma once
#include "ColliderShape.h"
#include"Utility/Vector3.h"

class SphereShape : public ColliderShape
{
public:
	SphereShape();
	SphereShape(const Vector3& pos,float radius);

	//半径を取得
	float GetRadius() const { return m_radius; }
	//半径を設定
	void SetRadius(float radius) { m_radius = radius; }

	//位置と半径を更新
	void Update(const Vector3& pos, float radius);
	//デバッグ用描画
	void Draw(unsigned int color, bool isFill = false) override;

	//位置を取得
	const Vector3 GetPos() const { return m_pos; }
	
	//球同士の当たり判定
	bool HitCollision(const SphereShape& other) const;

private:
	Vector3 m_pos;
	float m_radius;//半径
};

