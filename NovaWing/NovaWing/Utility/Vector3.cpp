#include "Vector3.h"
#include <cmath>

Vector3::Vector3():
	x(0.0f),y(0.0f),z(0.0f)
{

}


Vector3::Vector3(float x, float y, float z):
	x(x),y(y),z(z)
{
}

/// <summary>
/// ベクトルの大きさを計算する
/// </summary>
/// <returns>ベクトルの大きさ</returns>
float Vector3::Length() const
{
	return std::hypot(x, y, z);
}

void Vector3::Normalize()
{
	auto len = Length();
	if (len == 0.0f)
	{
		//0除算を避ける
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
		return;
	}

	//xとyをzlenで割ることで正規化する
	x /= len;
	y /= len;
	z /= len;
}

Vector3 Vector3::Normalized() const
{
	auto len = Length();
	if (len == 0.0f)
	{
		//0除算を避ける
		return { 0.0f,0.0f,0.0f };
	}
	return { x / len, y / len,z / len };
}

Vector3 Vector3::operator-() const
{
	/// ベクトルを反転したベクトルを返す
	return { -x,-y ,-z };
}

void Vector3::operator+=(const Vector3& val)
{
	/// ベクトルの加算
	x += val.x;
	y += val.y;
	z += val.z;
}

void Vector3::operator-=(const Vector3& val)
{
	/// ベクトルの減算
	x -= val.x;
	y -= val.y;
	z -= val.z;
}

void Vector3::operator*=(const float scale)
{
	/// ベクトルのスカラー倍
	x *= scale;
	y *= scale;
	z *= scale;
}

Vector3 Vector3::operator+(const Vector3& val) const
{
	/// ベクトルの加算
	return { x + val.x, y + val.y,z + val.z };
}

Vector3 Vector3::operator-(const Vector3& val) const
{
	/// ベクトルの減算
	return { x - val.x, y - val.y,z - val.z };
}

Vector3 Vector3::operator*(float scale) const
{
	/// ベクトルのスカラー倍
	return { x * scale, y * scale,z * scale };
}

bool Vector3::operator!=(const Vector3& val) const
{
	/// ベクトルの不等価比較
	return (x != val.x || y != val.y || z != val.z);
}

bool Vector3::operator==(const Vector3& val) const
{
	return (x == val.x && y == val.y && z == val.z);
}

VECTOR Vector3::ToDxLib() const
{
	//DxLibのVECTOR型に変換する
	return VGet(x, y, z);
}

void Vector3::Zero()
{
	x = 0.0f;
	y = 0.0f;
	z = 0.0f;
}

Vector3 Vector3::Lerp(const Vector3& start, const Vector3& end, float t)
{
	//線形補間
	return start + (end - start) * t;
}

float Vector3::Dot(const Vector3& a, const Vector3& b)
{
	return a.x + b.x + a.y + b.y + a.z + b.z;
}

Vector3 Vector3::FromWString(const std::wstring& x, const std::wstring& y, const std::wstring& z)
{
	//文字列をそのままfloatにして返す
	return Vector3(std::stof(x),std::stof(y),std::stof(z));
}
