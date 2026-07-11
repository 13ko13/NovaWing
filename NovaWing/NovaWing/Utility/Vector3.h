#pragma once
#include <DxLib.h>
#include <string>

/// <summary>
/// 3Dベクトル構造体
/// </summary>
struct Vector3
{
public:
	Vector3();
	Vector3(float x, float y, float z);

	float m_x, m_y, m_z;
	float Length()const;//ベクトルの大きさ
	void Normalize();//ベクトルの大きさを1にする
	Vector3 Normalized() const;///大きさを１にしたベクトルを返す
	Vector3 operator-()const;///ベクトルを反転したベクトルを返す
	//演算子オーバーロード
	void operator+=(const Vector3& val);///加算
	void operator-=(const Vector3& val);///減算
	void operator*=(const float scale);///ベクトルをn倍する
	Vector3 operator+(const Vector3& val)const;///加算
	Vector3 operator-(const Vector3& val)const;///減算
	Vector3 operator*(float scale)const;///ベクトルの乗算
	bool operator!=(const Vector3& val)const;///不等価比較
	bool operator==(const Vector3& val)const;///等価比較
	VECTOR ToDxLib() const;//DxLibのVECTOR型に変換する

	//Easing関数
	//勉強用
	//なぜstaticを付ける必要があるのか？
	//→staticを付けなかったら、わざわざVector3クラスのインスタンスを作成しないといけないから
	static Vector3 Lerp(const Vector3& start, const Vector3& end, float t);//線形補間

	//文字列(x,y,z)からVector3を生成する(CSV読み込みなどで使用する)
	static Vector3 FromWString(
		const std::wstring& x,
		const std::wstring& y,
		const std::wstring& z
	);
};

//別名(座標を表すこともあるため)
using Position3 = Vector3;

