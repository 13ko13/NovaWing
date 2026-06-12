#pragma once
#include <DxLib.h>

//前方宣言
class Vector3;

/// <summary>
/// 4x4行列クラス
/// </summary>
class Matrix4x4
{
public:
	Matrix4x4();

	Matrix4x4(float x0, float x1, float x2, float x3,
			  float y0, float y1, float y2, float y3,
			  float z0, float z1, float z2, float z3,
			  float w0, float w1, float w2, float w3);

	/// <summary>
	/// 行列の足し算
	/// </summary>
	/// <param name="right">右辺式</param>
	/// <returns></returns>
	Matrix4x4 operator+(const Matrix4x4& right) const;

	/// <summary>
	/// 行列の減算
	/// </summary>
	/// <param name="right">右辺式</param>
	/// <returns></returns>
	Matrix4x4 operator-(const Matrix4x4& right) const;

	/// <summary>
	/// 行列の掛け算
	/// </summary>
	/// <param name="r">右辺式</param>
	/// <returns></returns>
	Matrix4x4 operator*(const Matrix4x4& r) const;

	/// <summary>
	/// X軸回転行列
	/// </summary>
	/// <param name="angle">回転量</param>
	/// <returns></returns>
	static Matrix4x4 RotationX(float angle);

	/// <summary>
	/// Y軸回転行列
	/// </summary>
	/// <param name="angle">回転量</param>
	/// <returns></returns>
	static Matrix4x4 RotationY(float angle);

	/// <summary>
	/// Z軸回転行列
	/// </summary>
	/// <param name="angle">回転量</param>
	/// <returns></returns>
	static Matrix4x4 RotationZ(float angle);

	/// <summary>
	/// 平行移動行列
	/// </summary>
	/// <param name="right">右辺式</param>
	/// <returns></returns>
	static Matrix4x4 Translate(Vector3 right);

	/// <summary>
	/// 拡大縮小行列
	/// </summary>
	/// <param name="right"></param>
	/// <returns></returns>
	static Matrix4x4 Scale(Vector3 right);

	/// <summary>
	/// ベクトルに行列をかける
	/// </summary>
	/// <param name="vector">ベクトル</param>
	/// <returns>変換後のベクトル</returns>
	Vector3 Transform(const Vector3& vector) const;

	/// <summary>
	/// 自作Matrix4x4をDxLibのMATRIX型に変換する
	/// </summary>
	/// <returns></returns>
	MATRIX ToDxLib() const;

private:
	float m_x0, m_x1, m_x2, m_x3,
		  m_y0, m_y1, m_y2, m_y3,
		  m_z0, m_z1, m_z2, m_z3,
		  m_w0, m_w1, m_w2, m_w3;
};

