#include "Matrix4x4.h"
#include "Vector3.h"
#include <cmath>

Matrix4x4::Matrix4x4() :
	m_x0(0.0f), m_x1(0.0f), m_x2(0.0f), m_x3(0.0f),
	m_y0(0.0f), m_y1(0.0f), m_y2(0.0f), m_y3(0.0f),
	m_z0(0.0f), m_z1(0.0f), m_z2(0.0f), m_z3(0.0f),
	m_w0(0.0f), m_w1(0.0f), m_w2(0.0f), m_w3(0.0f)
{
}

Matrix4x4::Matrix4x4(
	float x0, float x1, float x2, float x3,
	float y0, float y1, float y2, float y3,
	float z0, float z1, float z2, float z3,
	float w0, float w1, float w2, float w3) :
	m_x0(x0), m_x1(x1), m_x2(x2), m_x3(x3),
	m_y0(y0), m_y1(y1), m_y2(y2), m_y3(y3),
	m_z0(z0), m_z1(z1), m_z2(z2), m_z3(z3),
	m_w0(w0), m_w1(w1), m_w2(w2), m_w3(w3)
{
}

Matrix4x4 Matrix4x4::operator+(const Matrix4x4& right) const
{
	return Matrix4x4(
		m_x0 + right.m_x0, m_x1 + right.m_x1, m_x2 + right.m_x2, m_x3 + right.m_x3,
		m_y0 + right.m_y0, m_y1 + right.m_y1, m_y2 + right.m_y2, m_y3 + right.m_y3,
		m_z0 + right.m_z0, m_z1 + right.m_z1, m_z2 + right.m_z2, m_z3 + right.m_z3,
		m_w0 + right.m_w0, m_w1 + right.m_w1, m_w2 + right.m_w2, m_w3 + right.m_w3
	);
}

Matrix4x4 Matrix4x4::operator-(const Matrix4x4& right) const
{
	return Matrix4x4(
		m_x0 - right.m_x0, m_x1 - right.m_x1, m_x2 - right.m_x2, m_x3 - right.m_x3,
		m_y0 - right.m_y0, m_y1 - right.m_y1, m_y2 - right.m_y2, m_y3 - right.m_y3,
		m_z0 - right.m_z0, m_z1 - right.m_z1, m_z2 - right.m_z2, m_z3 - right.m_z3,
		m_w0 - right.m_w0, m_w1 - right.m_w1, m_w2 - right.m_w2, m_w3 - right.m_w3
	);
}

Matrix4x4 Matrix4x4::operator*(const Matrix4x4& r) const
{

	Matrix4x4 out;

	out.m_x0 = m_x0 * r.m_x0 + m_x1 * r.m_y0 + m_x2 * r.m_z0 + m_x3 * r.m_w0;
	out.m_x1 = m_x0 * r.m_x1 + m_x1 * r.m_y1 + m_x2 * r.m_z1 + m_x3 * r.m_w1;
	out.m_x2 = m_x0 * r.m_x2 + m_x1 * r.m_y2 + m_x2 * r.m_z2 + m_x3 * r.m_w2;
	out.m_x3 = m_x0 * r.m_x3 + m_x1 * r.m_y3 + m_x2 * r.m_z3 + m_x3 * r.m_w3;

	out.m_y0 = m_y0 * r.m_x0 + m_y1 * r.m_y0 + m_y2 * r.m_z0 + m_y3 * r.m_w0;
	out.m_y1 = m_y0 * r.m_x1 + m_y1 * r.m_y1 + m_y2 * r.m_z1 + m_y3 * r.m_w1;
	out.m_y2 = m_y0 * r.m_x2 + m_y1 * r.m_y2 + m_y2 * r.m_z2 + m_y3 * r.m_w2;
	out.m_y3 = m_y0 * r.m_x3 + m_y1 * r.m_y3 + m_y2 * r.m_z3 + m_y3 * r.m_w3;

	out.m_z0 = m_z0 * r.m_x0 + m_z1 * r.m_y0 + m_z2 * r.m_z0 + m_z3 * r.m_w0;
	out.m_z1 = m_z0 * r.m_x1 + m_z1 * r.m_y1 + m_z2 * r.m_z1 + m_z3 * r.m_w1;
	out.m_z2 = m_z0 * r.m_x2 + m_z1 * r.m_y2 + m_z2 * r.m_z2 + m_z3 * r.m_w2;
	out.m_z3 = m_z0 * r.m_x3 + m_z1 * r.m_y3 + m_z2 * r.m_z3 + m_z3 * r.m_w3;

	out.m_w0 = m_w0 * r.m_x0 + m_w1 * r.m_y0 + m_w2 * r.m_z0 + m_w3 * r.m_w0;
	out.m_w1 = m_w0 * r.m_x1 + m_w1 * r.m_y1 + m_w2 * r.m_z1 + m_w3 * r.m_w1;
	out.m_w2 = m_w0 * r.m_x2 + m_w1 * r.m_y2 + m_w2 * r.m_z2 + m_w3 * r.m_w2;
	out.m_w3 = m_w0 * r.m_x3 + m_w1 * r.m_y3 + m_w2 * r.m_z3 + m_w3 * r.m_w3;

	return out;
}

Matrix4x4 Matrix4x4::RotationX(float angle)
{
	return Matrix4x4(
		1.0f,        0.0f,         0.0f, 0.0f,
		0.0f, cosf(angle), -sinf(angle), 0.0f,
		0.0f, sinf(angle),  cosf(angle), 0.0f,
		0.0f,        0.0f,         0.0f, 1.0f
	);
}

Matrix4x4 Matrix4x4::RotationY(float angle)
{
	return Matrix4x4(
		 cosf(angle),0.0f,sinf(angle),0.0f,
		        0.0f,1.0f,       0.0f,0.0f,
		-sinf(angle),0.0f,cosf(angle),0.0f,
		        0.0f,0.0f,       0.0f,1.0f
	);
}

Matrix4x4 Matrix4x4::RotationZ(float angle)
{
	return Matrix4x4(
		cosf(angle), -sinf(angle), 0.0f, 0.0f,
		sinf(angle),  cosf(angle), 0.0f, 0.0f,
		       0.0f,         0.0f, 1.0f, 0.0f,
		       0.0f,         0.0f, 0.0f, 1.0f
	);
}

Matrix4x4 Matrix4x4::Translate(Vector3 right)
{
	return Matrix4x4(
		     1.0f,      0.0f,      0.0f, 0.0f,
		     0.0f,      1.0f,      0.0f, 0.0f,
		     0.0f,      0.0f,      1.0f, 0.0f,
		right.m_x, right.m_y, right.m_z, 1.0f
	);
}

Matrix4x4 Matrix4x4::Scale(Vector3 right)
{
	return Matrix4x4(
		right.m_x,     0.0f,     0.0f, 0.0f,
		     0.0f,right.m_y,     0.0f, 0.0f,
		     0.0f,     0.0f,right.m_z, 0.0f,
		     0.0f,     0.0f,     0.0f, 1.0f
	);
}

Vector3 Matrix4x4::Transform(const Vector3& vector) const
{
	Vector3 result;
	
	result.m_x = vector.m_x * m_x0 + vector.m_y * m_y0 + vector.m_z * m_z0 + m_w0;
	result.m_y = vector.m_x * m_x1 + vector.m_y * m_y1 + vector.m_z * m_z1 + m_w1;
	result.m_z = vector.m_x * m_x2 + vector.m_y * m_y2 + vector.m_z * m_z2 + m_w2;

	return result;
}

MATRIX Matrix4x4::ToDxLib() const
{
	MATRIX mat;

	mat.m[0][0] = m_x0; mat.m[0][1] = m_x1; mat.m[0][2] = m_x2; mat.m[0][3] = m_x3;
	mat.m[1][0] = m_y0; mat.m[1][1] = m_y1; mat.m[1][2] = m_y2; mat.m[1][3] = m_y3;
	mat.m[2][0] = m_z0; mat.m[2][1] = m_z1; mat.m[2][2] = m_z2; mat.m[2][3] = m_z3;
	mat.m[3][0] = m_w0; mat.m[3][1] = m_w1; mat.m[3][2] = m_w2; mat.m[3][3] = m_w3;

	return mat;
}

