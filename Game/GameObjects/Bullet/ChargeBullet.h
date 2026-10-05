#pragma once
#include "BulletBase.h"

class CameraBase;
class ChargeExplosionCollider;
class ChargeBullet : public BulletBase
{
public:
	ChargeBullet(
		const Vector3& pos,
		const Vector3& vel,
		int attackPower,
		std::weak_ptr<GameObject> pTarget,
		std::weak_ptr<CameraBase> pCamera,
		std::weak_ptr<EffectManager> pEffectManager);
	~ChargeBullet();

	void Update() override;//更新処理
	void Draw() override;//描画処理

	//敵にヒットした時の処理
	void OnHitEnemy(ColliderTag hitTag) override;

	//まだ当たれるか
	//弾が生きてて、爆発中じゃなければまだ当たれる
	bool CanHit() const override { return !IsDead() && !m_isExploding; }

	//爆発用のコライダーを取得
	ICollider& GetExplosionCollider();
	//爆発用の球を取得
	std::shared_ptr<SphereShape> GetExplosionSphere() const { return m_pExplosionSphere; }
	//爆発中か
	bool IsExplosionActive() const { return m_isExploding && m_explosionFrame == 0; }

private:
	//ターゲットのオブジェクト
	std::weak_ptr<GameObject> m_pTarget;

	//受け取った弾の速度を保存しておく
	float m_speed = 0.0f;

	//Effekseerのエフェクト再生中のハンドル
	int m_effectPlayHandle = -1;

	//爆発中か
	bool m_isExploding = false;
	//爆発してからのフレーム
	int m_explosionFrame = 0;
	//爆発用の大きい球
	std::shared_ptr<SphereShape> m_pExplosionSphere;
	//爆発のコライダー
	std::unique_ptr<ChargeExplosionCollider> m_pExplosionCollider;
};