#pragma once
#include "Game/Collision/ICollider.h"

class GunshipBoss;
class GunshipBossNetState;
class GunshipBossNetCollider : public ICollider
{
public:
	GunshipBossNetCollider(GunshipBoss& owner);
	virtual ~GunshipBossNetCollider();

	//当たり判定の形状取得
	std::vector<std::shared_ptr<ColliderShape>> GetCollision() const override;
	//自分のタグを取得
	ColliderTag GetTag() const override;
	//衝突時処理
	void OnCollision(const ICollider& other) override;
	//判定が有効かどうか
	bool IsCollisionActive() const override;

	//持ち主のIDを取得(多段ヒット防止の識別に使う)
	int GetOwnerID() const;

private:
	GunshipBoss& m_owner;
};
