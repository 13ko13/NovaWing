#pragma once
#include "Game/Collision/ICollider.h"

class GunshipBoss;
/// <summary>
/// 2面のボスの弱点(放熱中に開くポッドの中)の判定。弾が当たるとダメージを受ける
/// </summary>
class GunshipBossWeakCollider : public ICollider
{
public:
	GunshipBossWeakCollider(GunshipBoss& owner);
	virtual ~GunshipBossWeakCollider();

	//当たり判定の形状取得
	std::vector<std::shared_ptr<ColliderShape>> GetCollision() const override;
	//自分のタグを取得
	ColliderTag GetTag() const override;
	//衝突時処理
	void OnCollision(const ICollider& other) override;
	//判定が有効かどうか
	bool IsCollisionActive() const override;

private:
	GunshipBoss& m_owner;
};
