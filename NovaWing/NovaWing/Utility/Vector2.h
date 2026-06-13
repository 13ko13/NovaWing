#pragma once
#include <DxLib.h>

/// <summary>
/// 2Dベクトル構造体
/// </summary>
struct Vector2
{
public:
	Vector2();
	Vector2(float x, float y);

	float m_x, m_y;
	float Length()const;//ベクトルの大きさ
	void Normalize();//ベクトルの大きさを1にする
	Vector2 Normalized() const;///大きさを１にしたベクトルを返す
	Vector2 operator-()const;///ベクトルを反転したベクトルを返す
	//演算子オーバーロード
	void operator+=(const Vector2& val);///加算
	void operator-=(const Vector2& val);///減算
	void operator*=(const float scale);///ベクトルをn倍する
	Vector2 operator+(const Vector2& val)const;///加算
	Vector2 operator-(const Vector2& val)const;///減算
	Vector2 operator*(float scale)const;///ベクトルの乗算
	bool operator!=(const Vector2& val)const;///不等価比較
	bool operator==(const Vector2& val)const;///等価比較
	VECTOR ToDxLib() const;//DxLibのVECTOR型に変換する

	//Easing関数
	//勉強用
	//なぜstaticを付ける必要があるのか？→staticを付けなかったら、
	//わざわざVector2クラスのインスタンスを作成しないといけないから
	static Vector2 Lerp(const Vector2& start, const Vector2& end, float t);//線形補間

	//別名(座標を表すこともあるため)
	using Position2 = Vector2;
};