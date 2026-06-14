#pragma once
#include <memory>

#include "../Utility/Vector3.h"
#include "../Utility/Quaternion.h"

class ModelAnimator;
class Actor
{
public:
	Actor();
	virtual ~Actor();

	//純粋仮想関数
	virtual void Update() = 0;//更新
	virtual void Draw() = 0;//描画

	//セッター
	void SetPos(const Position3& pos) { m_pos = pos; }//位置を指定
	void SetVel(const Vector3& vel) { m_velocity = vel; };//速度を指定
	void SetRotation(const Quaternion& rot) { m_rotation = rot; };//回転を指定

	//ゲッター
	Position3 GetPos() const { return m_pos; }//位置を取得
	Vector3 GetVel() const { return m_velocity; }//速度を取得
	Quaternion GetRotation() const { return m_rotation; }//回転を取得
	bool IsDead() const { return m_isDead; }//死んでいるかを返す

	//操作
	void Rotate(const Vector3& axis, float angle);//回転を加える

private:

protected:
	//機体情報
	int m_modelHandle = -1;//モデルハンドル
	bool m_isDead = false;//死んでいるか
	Position3 m_pos;//3D座標
	Quaternion m_rotation;//回転
	Vector3 m_velocity;//速度

	//モデルのアニメーター
	std::shared_ptr<ModelAnimator> m_pAnimator;
};

