#pragma once
#include "Vector3.h"

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

	//クォータニオン同士の演算
	Quaternion operator*(const Quaternion& other) const;//クォータニオンの乗算
	Vector3 operator*(const Vector3& vec) const;//クォータニオンとベクトルの乗算(回転)
};

