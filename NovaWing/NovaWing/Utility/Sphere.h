#pragma once
#include "Vector3.h"

class Sphere
{
public:
	Sphere(const Vector3& pos);
	~Sphere();

	void Update(const Vector3& pos, const float radius);
	void Draw(unsigned int color);

	/// <summary>
	/// 位置を返す
	/// </summary>
	/// <returns>球の位置</returns>
	const Vector3 GetPos() const { return m_pos; }

	/// <summary>
	/// 球の半径を返す
	/// </summary>
	/// <returns>球の半径</returns>
	const float GetRadius() const { return m_radius; }

	/// <summary>
	/// 球との球の当たり判定
	/// </summary>
	/// <param name="other">指定の球</param>
	/// <returns>true:当たっている、false:当たっていない</returns>
	bool HitCollision(const Sphere& other) const;

private:
	//位置
	Vector3 m_pos;

	//半径
	float m_radius = 0.0f;
};