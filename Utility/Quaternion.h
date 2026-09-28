#pragma once
#include "Vector3.h"

class Matrix4x4;
class Quaternion
{
public:
	float m_w,//実部
		m_x,//虚部
		m_y,//虚部
		m_z;//虚部

	//コンストラクタ
	Quaternion();//単位元(何もしないクォータニオン)を作成
	Quaternion(float w, float x, float y, float z);//直接指定してクォータニオンを作成
	Quaternion(const Vector3& axis, float angle);//軸と角度からクォータニオンを作成

	//操作
	Quaternion Normalize();//クォータニオンを正規化する
	Quaternion Inverse() const;//クォータニオンの逆数を計算する
	float Length() const;//長さを求める
	static Quaternion Lerp(const Quaternion& q1, const Quaternion& q2, float t);//線形補完

	//クォータニオン同士の演算
	Quaternion operator*(const Quaternion& other) const;//クォータニオンの乗算
	Vector3 operator*(const Vector3& vec) const;//クォータニオンとベクトルの乗算(回転)

	//Matrix4x4に変換する
	Matrix4x4 ToMatrix4x4() const;
};

