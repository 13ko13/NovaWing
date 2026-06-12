#include <cmath>

#include "Quaternion.h"

Quaternion::Quaternion() :
	m_w(1.0f), m_x(0.0f), m_y(0.0f), m_z(0.0f)
{
}

Quaternion::Quaternion(float w, float x, float y, float z) :
	m_w(w), m_x(x), m_y(y), m_z(z)
{
}

Quaternion::Quaternion(const Vector3& axis, float angle)
{
	//それぞれの成分を軸と角度から公式を使用して求める
	m_w = std::cosf(angle / 2);
	m_x = std::sinf(angle / 2) * axis.m_x;
	m_y = std::sinf(angle / 2) * axis.m_y;
	m_z = std::sinf(angle / 2) * axis.m_z;
}

Quaternion Quaternion::Normalize() 
{
	Quaternion q;
	//長さを求める
	float length = Length();
	//0除算を避けるため
	if (length == 0.0f)
	{
		//単位元を返す
		return Quaternion();
	}
	//正規化するため、成分を長さで割る
	q.m_w = m_w / length;
	q.m_x = m_x / length;
	q.m_y = m_y / length;
	q.m_z = m_z / length;

	return q;
}

Quaternion Quaternion::Inverse() const
{
	//虚部の符号を反転させる
	return Quaternion(m_w, m_x, m_y, m_z);
}

float Quaternion::Length() const
{
	//Quaternionの長さを求める
	float len = 0.0f;
	len = std::sqrtf(m_w * m_w +
					m_x * m_x +
					m_y * m_y +
					m_z * m_z);

	return len;
}

Quaternion Quaternion::operator*(const Quaternion& other) const
{
	Quaternion q;
	//それぞれを公式に基づいて計算する
	q.m_w = m_w * other.m_w - m_x * other.m_x - m_y * other.m_y - m_z * other.m_z;
	q.m_x = m_w * other.m_x + m_x * other.m_w + m_y * other.m_z - m_z * other.m_y;
	q.m_y = m_w * other.m_y - m_x * other.m_z + m_y * other.m_w + m_z * other.m_x;
	q.m_z = m_w * other.m_z + m_x * other.m_y - m_y * other.m_x + m_z * other.m_w;

	return q;
}

Vector3 Quaternion::operator*(const Vector3& vec) const
{
	//Vector3をクォータニオンに変換する
	Quaternion p(0.0f, vec.m_x, vec.m_y, vec.m_z);

	//q*pを計算
	Quaternion temp = (*this) * p;

	//temp*q.inverse()を計算
	Quaternion result = temp * this->Inverse();

	//結果の虚部だけを取り出して、Vector3で返す
	return Vector3(result.m_x, result.m_y, result.m_z);
}
