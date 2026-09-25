#pragma once
#include "ICollider.h"

class EnemyBase;
/// <summary>
/// 雑魚敵(浮遊敵・ワーム)のコライダー
/// </summary>
class EnemyCollider : public ICollider
{
public:
	EnemyCollider(EnemyBase& owner, ColliderTag tag);

	//当たり判定の形状データを取得
	std::vector<std::shared_ptr<ColliderShape>> GetCollision() const override;
	//自分のタグを取得
	ColliderTag GetTag() const override { return m_tag; }
	//衝突時処理
	void OnCollision(const ICollider& other) override;
	//判定が有効かどうか
	bool IsCollisionActive() const override;

	//持ち主のIDを取得(多段ヒット防止の識別に使う)
	int GetOwnerID() const;

private:
	//敵を参照したい
	EnemyBase& m_owner;
	//接触ダメージのない敵(Enemy)か、ワーム(Worm)か
	ColliderTag m_tag;
};
