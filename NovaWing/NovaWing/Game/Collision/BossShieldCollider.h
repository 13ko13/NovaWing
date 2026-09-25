#pragma once
#include "ICollider.h"

class BossEnemy;
/// <summary>
/// ボスの無敵判定(弾を弾いてシールドエフェクトを出す部分)
/// </summary>
class BossShieldCollider : public ICollider
{
public:
	BossShieldCollider(BossEnemy& owner);

	//当たり判定の形状データを取得
	std::vector<std::shared_ptr<ColliderShape>> GetCollision() const override;
	//自分のタグを取得
	ColliderTag GetTag() const override { return ColliderTag::BossShield; }
	//衝突時処理
	void OnCollision(const ICollider& other) override;
	//判定が有効かどうか
	bool IsCollisionActive() const override;

private:
	//ボスを参照したい
	BossEnemy& m_owner;
};
