#pragma once
#include "EnemyBullet.h"
#include "Utility/Vector3.h"

class SpiralBullet : public EnemyBullet
{
public:
	SpiralBullet(const Vector3& pos, const Vector3& vel,
		const int attackPower, std::weak_ptr<CameraBase> pCamera,
		std::shared_ptr<EnemyBase> pShooter,
		std::weak_ptr<EffectManager> pEffectManager,
		float startAngle);
	virtual ~SpiralBullet();

	//更新処理
	void Update() override;

private:
	//軸に合わせて回転する用の弾の位置
	Vector3 m_spiralPos;

	//弾の角度
	float m_bulletAngle = 0.0f;
};
