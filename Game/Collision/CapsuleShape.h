#pragma once
#include "ColliderShape.h"
#include "Utility/Vector3.h"

class SphereShape;
class CapsuleShape : public ColliderShape
{
public:
	CapsuleShape(const Vector3& startPos, const Vector3& endPos, float radius);
	virtual ~CapsuleShape();

	//両端と半径を更新
	void Update(const Vector3& startPos, const Vector3& endPos, float radius);
	//デバッグ用描画
	void Draw(unsigned int color, bool isFill = false) override;

	//始点を取得
	const Vector3& GetStartPos() const { return m_startPos; }
	//終点を取得
	const Vector3& GetEndPos() const { return m_endPos; }
	//半径を取得
	float GetRadius() const { return m_radius; }

	//球との当たり判定
	bool HitCollision(const SphereShape& sphere) const;

private:
	Vector3 m_startPos;//線分の始点
	Vector3 m_endPos;//線分の終点
	float m_radius;//半径(線分からの太さ)
};
