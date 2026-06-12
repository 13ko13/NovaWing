#pragma once
#include "Vector3.h"
#include "Quaternion.h"

class Actor
{
public:
	Actor();
	virtual ~Actor();

	//純粋仮想関数
	virtual void Update() = 0;//更新
	virtual void Draw() = 0;//描画
	virtual void TakeDamage(int damage) = 0;//ダメージを受ける

	//セッター
	void SetPos(const Position3& pos) { m_position = pos; }//位置を指定
	void SetVel(const Vector3& vel) { m_velocity = vel; };//速度を指定
	void SetRotation(const Quaternion& rot) { m_rotation = rot; };//回転を指定

	//ゲッター
	Position3 GetPos() const { return m_position; }//位置を取得
	Vector3 GetVel() const { return m_velocity; }//速度を取得
	Quaternion GetRotation() const { return m_rotation; }//回転を取得
	int GetHealth() const { return m_health; }//HPを取得

	//操作
	void Rotate(const Vector3& axis, float angle);//回転を加える
	bool IsDead() const;//死の条件

private:

protected:
	Position3 m_position;//3D座標
	Quaternion m_rotation;//回転
	Vector3 m_velocity;//速度
	int m_health;//体力
	bool m_isDead;//死んでいるか
};

