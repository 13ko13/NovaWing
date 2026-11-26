#pragma once
#include "EnemyBullet.h"
#include "Utility/Vector3.h"

class SpiralBullet : public EnemyBullet
{
public:
	//回転する弾に必要なデータ
	struct SpiralBulletData
	{
		Vector3 pos;
		Vector3 vel;
		int attackPower;
		std::weak_ptr<CameraBase> pCamera;
		std::weak_ptr<EnemyBase> pShooter;
		float startAngle;
	};
	SpiralBullet(const SpiralBulletData& data, std::weak_ptr<EffectManager> pEffectManager);
	virtual ~SpiralBullet();

	//更新処理
	void Update() override;

private:
	//軸に合わせて回転する用の弾の位置
	Vector3 m_spiralPos;

	//弾の角度
	float m_bulletAngle = 0.0f;
};
