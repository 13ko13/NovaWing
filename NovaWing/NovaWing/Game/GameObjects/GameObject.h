#pragma once
#include <memory>

#include "../../Utility/Vector3.h"
#include "../../Utility/Quaternion.h"

/// <summary>
/// 位置を持つ全てのオブジェクト
/// </summary>
class GameObject:
	public std::enable_shared_from_this<GameObject>//自分自身のポインタを返すため
{
public:
	GameObject();
	virtual ~GameObject();

	virtual void Init() final;
	//Actorを継承したクラスが独自のInit処理を書く
	virtual void OnInit() {};
	virtual void Update() = 0;
	virtual void Draw() = 0;

	//セッター
	void SetPos(const Position3& pos) { m_pos = pos; }//位置を指定
	void SetRotation(const Quaternion& rot) { m_rotation = rot; };//回転を指定

	Position3 GetPos() const { return m_pos; }//位置を取得
	Quaternion GetRotation() const { return m_rotation; }//回転を取得
	bool IsDead() const { return m_isDead; }//死んでいるかを返す


	//操作
	void Rotate(const Vector3& axis, float angle);//回転を加える
protected:
	bool m_isDead = false;//死んでいるか

	Position3 m_pos;
	Quaternion m_rotation;
};

