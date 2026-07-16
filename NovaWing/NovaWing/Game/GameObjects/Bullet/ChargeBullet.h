#pragma once
#include "BulletBase.h"
class ChargeBullet :  public BulletBase
{
public:
	ChargeBullet(const Vector3& pos, const Vector3& vel, int attackPower,std::weak_ptr<GameObject> pTarget);
	~ChargeBullet();

	void Update() override;//更新処理
	void Draw() override;//描画処理

	//敵にヒットした時の処理
	void OnHitEnemy() override;

private:
	//ターゲットのオブジェクト
	std::weak_ptr<GameObject> m_pTarget;

	//受け取った弾の速度を保存しておく
	float m_speed = 0.0f;

	//Effekseerのエフェクト再生中のハンドル
	int m_effectPlayHandle = -1;
};