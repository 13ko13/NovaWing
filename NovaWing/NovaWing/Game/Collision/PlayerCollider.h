#pragma once
#include "ICollider.h"
#include "Utility/Vector3.h"
#include "SphereShape.h"

class Player;
/// <summary>
/// プレイヤーのコライダー
/// </summary>
class PlayerCollider : public ICollider
{
public:
	PlayerCollider(Player& owner);

	//当たり判定の形状データを取得
	std::vector<std::shared_ptr<ColliderShape>> GetCollision() const override;
	//自分のタグを取得
	ColliderTag GetTag() const override { return ColliderTag::Player; }
	//衝突時処理
	void OnCollision(const ICollider& other) override;
	//判定が有効かどうか
	bool IsCollisionActive() const override;

	//球の位置、半径を更新
	void UpdateShape(const Vector3& pos, float radius);
	//球そのものを取得
	std::shared_ptr<SphereShape> GetSphere() const { return m_sphere; }

private:
	//プレイヤーを参照したい
	Player& m_owner;
	//当たり判定用の球
	std::shared_ptr<SphereShape> m_sphere;
};

