#pragma once
#include <memory>
#include <vector>

#include "ColliderShape.h"

/// <summary>
/// 衝突判定で使用するオブジェクトそれぞれのタグ
/// </summary>
enum class ColliderTag
{
	Player,//プレイヤー
	Enemy,//敵(接触してもダメージのない敵)
	Worm,//ワームエネミー(接触するとプレイヤーにダメージ)
	BossDamage,//ボスのダメージ判定
	BossShield,//ボスの無敵判定(弾を弾く)
	BossBeam,//ボスのビーム
	Rock,//岩
	EnemyBullet,//敵弾
	PlayerBullet,//プレイヤーの弾
	Counter,//カウンター
};

/// <summary>
/// 当たり判定を持つオブジェクトが実装するインターフェース
/// </summary>
class ICollider
{
public:
	ICollider() {};
	virtual ~ICollider() = default;

	//当たり判定の形状取得
	virtual std::vector<std::shared_ptr<ColliderShape>> GetCollision() const = 0;
	//自分のタグを取得
	virtual ColliderTag GetTag() const = 0;
	//衝突時処理
	virtual void OnCollision(const ICollider& other) = 0;
	//判定が有効化どうか
	virtual bool IsCollisionActive() const = 0;
};

