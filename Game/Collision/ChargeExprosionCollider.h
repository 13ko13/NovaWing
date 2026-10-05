#pragma once
#include "ICollider.h"

class BulletBase;
class ChargeExprosionCollider :public ICollider
{
public:
	ChargeExprosionCollider(BulletBase& owner, ColliderTag tag);

	//当たり判定の形状データを取得
	std::vector<std::shared_ptr<ColliderShape>> GetCollision() const override;
	//自分のタグを取得
	ColliderTag GetTag() const override { return m_tag; }
	//衝突時処理
	void OnCollision(const ICollider& other) override;
	//判定が有効かどうか
	bool IsCollisionActive() const override;

	//弾の攻撃力を取得
	int GetAttackPower() const;
	//弾の位置を取得
	Vector3 GetPos() const;

	//参照先を取得
	BulletBase& GetOwner() const;

private:
	//弾を参照したい
	BulletBase& m_owner;
	//プレイヤー側の弾か敵弾か
	ColliderTag m_tag;
};
