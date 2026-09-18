#pragma once
#include "ICollider.h"

class Player;
/// <summary>
/// プレイヤーのコライダー
/// </summary>
class PlayerCollider : public ICollider
{
public:
	PlayerCollider(Player& owner);

	//当たり判定の形状データを取得
	std::shared_ptr<ColliderShape> GetCollision() const override;
	//自分のタグを取得
	ColliderTag GetTag() const override { return ColliderTag::Player; }
	//衝突時処理
	void OnCollision(const ICollider& other) override;
	//判定が有効かどうか
	bool IsCollisionActive() const override;

private:
	//プレイヤーを参照したい
	Player& m_owner;
};

