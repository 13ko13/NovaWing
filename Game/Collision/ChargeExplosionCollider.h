#pragma once
#include "ICollider.h"
#include "Utility/Vector3.h"

class ChargeBullet;
class ChargeExplosionCollider :public ICollider
{
public:
	ChargeExplosionCollider(ChargeBullet& owner);
	~ChargeExplosionCollider();

	//当たり判定の形状データを取得
	std::vector<std::shared_ptr<ColliderShape>> GetCollision() const override;
	//タグを取得
	ColliderTag GetTag() const override { return ColliderTag::ChargeExplosion; }
	//衝突時処理
	void OnCollision(const ICollider& other) override;
	//判定が有効かどうか
	bool IsCollisionActive() const override;

	//爆発の攻撃力を取得
	int GetAttackPower() const;

private:
	//弾を参照したい
	ChargeBullet& m_owner;
};
