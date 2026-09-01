#pragma once

#include "Manager/ResourceLoader.h"
#include "Game/GameObjects/GameObject.h"
#include "Utility/Sphere.h"

class CameraBase;
class BulletBase : public GameObject
{
public:
	BulletBase(
		const Vector3& pos,
		const Vector3& vel,
		int attackPower,
		float radius,
		std::weak_ptr<CameraBase> pCamera
	);
	virtual ~BulletBase();

	virtual void Update();//更新処理
	virtual void Draw();//描画処理

	//当たり判定用の球を取得
	const Sphere& GetSphere() const { return m_sphere; }

	//攻撃力を取得
	const int GetAttackPower() const { return m_attackPower; }

	//相手に弾が当たった時の処理
	virtual void OnHitEnemy();

protected:
	//当たり判定用の球
	Sphere m_sphere;

	//カメラ
	std::weak_ptr<CameraBase> m_pCamera;

	//攻撃力
	int m_attackPower = 0;
	//生存時間
	int m_lifeTime = 0;
	//球の半径
	float m_radius = 0.0f;

	//ヒットエフェクト再生ハンドル
	int m_hitEffectPlayH = -1;
};

