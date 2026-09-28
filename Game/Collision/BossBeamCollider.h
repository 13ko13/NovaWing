#pragma once
#include "ICollider.h"

class BossEnemy;
/// <summary>
/// ボスのビーム(左右まとめて1つのダメージ源として扱う)
/// </summary>
class BossBeamCollider : public ICollider
{
public:
	BossBeamCollider(BossEnemy& owner);

	//当たり判定の形状データを取得
	std::vector<std::shared_ptr<ColliderShape>> GetCollision() const override;
	//自分のタグを取得
	ColliderTag GetTag() const override { return ColliderTag::BossBeam; }
	//衝突時処理
	void OnCollision(const ICollider& other) override;
	//判定が有効かどうか(ビームステート中のみ有効)
	bool IsCollisionActive() const override;

	//ビームのダメージを取得
	int GetDamage() const;
	//持ち主のIDを取得(多段ヒット防止の識別に使う)
	int GetOwnerID() const;

private:
	//ボスを参照したい
	BossEnemy& m_owner;
};
