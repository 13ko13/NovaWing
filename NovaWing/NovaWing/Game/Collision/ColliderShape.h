#pragma once

/// <summary>
/// 当たり判定の形の種類
/// </summary>
enum class Shape
{
	Sphere,//球
};

/// <summary>
/// 当たり判定の形に関する基底クラス
/// </summary>
class ColliderShape
{
public:
	ColliderShape(Shape shape) : m_shape(shape) {};
	virtual ~ColliderShape() = default;

	//当たり判定の形の種類を取得
	Shape GetShape() const { return m_shape; }

private:
	Shape m_shape;//当たり判定の形
};