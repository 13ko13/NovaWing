#pragma once
#include "BulletBase.h"

class CameraBase;
class EnemyBase;
class EnemyBullet : public BulletBase
{
public:
	EnemyBullet(const Vector3& pos, const Vector3& vel,
		const int attackPower,std::weak_ptr<CameraBase> pCamera,
		std::shared_ptr<EnemyBase> pShooter);
	~EnemyBullet();

	void Update() override;//更新処理
	void Draw() override;//描画処理

	//弾がプレイヤーに当たった時の処理
	void OnHitEnemy() override;

	//発射元のエネミーを取得
	std::weak_ptr<EnemyBase> GetShooter() const { return m_pShooter; }

private:
	//Effekseerのエフェクト再生中のハンドル
	int m_effectPlayHandle = -1;
	//発射元のエネミーを持っておく(プレイヤーのカウンター対象)
	std::weak_ptr<EnemyBase> m_pShooter;
};