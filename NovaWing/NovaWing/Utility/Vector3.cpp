#include "Vector3.h"
#include <cmath>

Vector3::Vector3():
	m_x(0.0f),m_y(0.0f),m_z(0.0f)
{

}


Vector3::Vector3(float x, float y, float z):
	m_x(x),m_y(y),m_z(z)
{
}

/// <summary>
/// ベクトルの大きさを計算する
/// </summary>
/// <returns>ベクトルの大きさ</returns>
float Vector3::Length() const
{
	return std::hypot(m_x, m_y, m_z);
}

void Vector3::Normalize()
{
	auto len = Length();
	if (len == 0.0f)
	{
		//0除算を避ける
		m_x = 0.0f;
		m_y = 0.0f;
		m_z = 0.0f;
		return;
	}

	//xとyをzlenで割ることで正規化する
	m_x /= len;
	m_y /= len;
	m_z /= len;
}

Vector3 Vector3::Normalized() const
{
	auto len = Length();
	if (len == 0.0f)
	{
		//0除算を避ける
		return { 0.0f,0.0f,0.0f };
	}
	return { m_x / len, m_y / len,m_z / len };
}

Vector3 Vector3::operator-() const
{
	/// ベクトルを反転したベクトルを返す
	return { -m_x,-m_y ,-m_z };
}

void Vector3::operator+=(const Vector3& val)
{
	/// ベクトルの加算
	m_x += val.m_x;
	m_y += val.m_y;
	m_z += val.m_z;
}

void Vector3::operator-=(const Vector3& val)
{
	/// ベクトルの減算
	m_x -= val.m_x;
	m_y -= val.m_y;
	m_z -= val.m_z;
}

void Vector3::operator*=(const float scale)
{
	/// ベクトルのスカラー倍
	m_x *= scale;
	m_y *= scale;
	m_z *= scale;
}

Vector3 Vector3::operator+(const Vector3& val) const
{
	/// ベクトルの加算
	return { m_x + val.m_x, m_y + val.m_y,m_z + val.m_z };
}

Vector3 Vector3::operator-(const Vector3& val) const
{
	/// ベクトルの減算
	return { m_x - val.m_x, m_y - val.m_y,m_z - val.m_z };
}

Vector3 Vector3::operator*(float scale) const
{
	/// ベクトルのスカラー倍
	return { m_x * scale, m_y * scale,m_z * scale };
}

bool Vector3::operator!=(const Vector3& val) const
{
	/// ベクトルの不等価比較
	return (m_x != val.m_x || m_y != val.m_y || m_z != val.m_z);
}

bool Vector3::operator==(const Vector3& val) const
{
	return (m_x == val.m_x && m_y == val.m_y && m_z == val.m_z);
}

VECTOR Vector3::ToDxLib() const
{
	//DxLibのVECTOR型に変換する
	return VGet(m_x, m_y, m_z);
}

Vector3 Vector3::Lerp(const Vector3& start, const Vector3& end, float t)
{
	//線形補間
	return start + (end - start) * t;
}

float Vector3::Dot(const Vector3& a, const Vector3& b)
{
	return a.m_x + b.m_x + a.m_y + b.m_y + a.m_z + b.m_z;
}

Vector3 Vector3::FromWString(const std::wstring& x, const std::wstring& y, const std::wstring& z)
{
	//文字列をそのままfloatにして返す
	return Vector3(std::stof(x),std::stof(y),std::stof(z));
}
