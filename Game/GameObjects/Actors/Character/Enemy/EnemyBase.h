#pragma once
#include <memory>
#include <vector>

#include "Game/GameObjects/Actors/Character/Character.h"
#include "Game/Collision/SphereShape.h"

class Player;
class BulletManager;
class CameraBase;
class SoundManager;
class EffectManager;
class ICollider;
class EnemyBase : public Character
{
public:
	EnemyBase(
		ResourceLoader::ModelID modelID,
		std::weak_ptr<CameraBase> pCamera,
		std::weak_ptr<Player> pPlayer,
		std::weak_ptr<BulletManager> pBulletManager,
		std::weak_ptr<SoundManager> pSoundManager,
		std::weak_ptr<EffectManager> pEffectManager
	);
	virtual ~EnemyBase();

	//ダメージを受ける
	//体力が0以下になったら死亡待機に入る(死に方が違う敵はoverrideする)
	void TakeDamage(int damage) override;

	//最後にダメージを与えた攻撃の種類(何で倒されたかをプレイヤーに伝えるため)
	enum class DamageSource
	{
		None,//まだダメージを受けていない、または弾以外
		NormalShot,//通常弾(反射弾も含む)
		ChargeShot,//チャージショットの爆発
	};
	//最後にダメージを与えた攻撃の種類を設定する
	void SetLastDamageSource(DamageSource source) { m_lastDamageSource = source; }

	//死亡待機状態かを返す
	bool IsDying() const { return m_isDying; }

	//死んだときに呼ぶ関数
	void OnEnemyDead();

	//当たり判定に使う球の配列を返す
	//当たり判定を持つ敵がoverrideする
	virtual std::vector<std::shared_ptr<SphereShape>> GetCollisionSpheres() const { return {}; }

	//この敵が持つ当たり判定インターフェースをすべて返す
	virtual std::vector<ICollider*> GetColliders() { return {}; }

	virtual void OnHitPlayer() {};

private:

protected:
	//プレイヤーの弱参照
	std::weak_ptr<Player> m_pPlayer;
	//バレットマネージャーの弱参照
	std::weak_ptr<BulletManager> m_pBulletManager;
	//サウンドマネージャーの弱参照
	std::weak_ptr<SoundManager> m_pSoundManager;
	//エフェクトマネージャーの弱参照
	std::weak_ptr<EffectManager> m_pEffectManager;
	//死亡待機状態か
	bool m_isDying = false;
	//当たり判定球の半径
	float m_colRadius = 0.0f;
	//本当に死ぬまでのフレーム
	int m_trueDeadFrame = 0;
	//死亡待機の時間を数える
	int m_dyingFrame = 0;
	//最後にダメージを与えた攻撃の種類
	DamageSource m_lastDamageSource = DamageSource::None;
};

