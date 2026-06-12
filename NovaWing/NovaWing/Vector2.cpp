#include "Vector2.h"
#include <cmath>

Vector2::Vector2() :
	m_x(0.0f), m_y(0.0f)
{

}


Vector2::Vector2(float x, float y) :
	m_x(x), m_y(y)
{
}

/// <summary>
/// ベクトルの大きさを計算する
/// </summary>
/// <returns>ベクトルの大きさ</returns>
float Vector2::Length() const
{
	return std::hypot(m_x, m_y );
}

void Vector2::Normalize()
{
	auto len = Length();
	if (len == 0.0f)
	{
		//0除算を避ける
		m_x = 0.0f;
		m_y = 0.0f;
		return;
	}

	//xとyをzlenで割ることで正規化する
	m_x /= len;
	m_y /= len;
}

Vector2 Vector2::Normalized() const
{
	auto len = Length();
	if (len == 0.0f)
	{
		//0除算を避ける
		return { 0.0f,0.0f };
	}
	return { m_x / len, m_y / len };
}

Vector2 Vector2::operator-() const
{
	/// ベクトルを反転したベクトルを返す
	return { -m_x,-m_y};
}

void Vector2::operator+=(const Vector2& val)
{
	/// ベクトルの加算
	m_x += val.m_x;
	m_y += val.m_y;
}

void Vector2::operator-=(const Vector2& val)
{
	/// ベクトルの減算
	m_x -= val.m_x;
	m_y -= val.m_y;
}

void Vector2::operator*=(const float scale)
{
	/// ベクトルのスカラー倍
	m_x *= scale;
	m_y *= scale;
}

Vector2 Vector2::operator+(const Vector2& val) const
{
	/// ベクトルの加算
	return { m_x + val.m_x, m_y + val.m_y };
}

Vector2 Vector2::operator-(const Vector2& val) const
{
	/// ベクトルの減算
	return { m_x - val.m_x, m_y - val.m_y};
}

Vector2 Vector2::operator*(float scale) const
{
	/// ベクトルのスカラー倍
	return { m_x * scale, m_y * scale };
}

bool Vector2::operator!=(const Vector2& val) const
{
	/// ベクトルの不等価比較
	return (m_x != val.m_x || m_y != val.m_y);
}

bool Vector2::operator==(const Vector2& val) const
{
	return (m_x == val.m_x && m_y == val.m_y);
}

VECTOR Vector2::ToDxLib() const
{
	//DxLibのVECTOR型に変換する
	return VGet(m_x, m_y, 0.0f);
}

Vector2 Vector2::Lerp(const Vector2& start, const Vector2& end, float t)
{
	//線形補間
	return start + (end - start) * t;
}