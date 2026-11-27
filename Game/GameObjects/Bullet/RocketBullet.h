#pragma once
#include "Game/GameObjects/Bullet/EnemyBullet.h"

class RocketModel;
class RocketBullet : public EnemyBullet
{
public:
	//RocketBulletに必要な引数
	struct RocketBulletData
	{
		Vector3 pos;
		Vector3 vel;
		int attackPower;
		std::weak_ptr<CameraBase> pCamera;
		std::shared_ptr<EnemyBase> pShooter;
		std::weak_ptr<EffectManager> pEffectManager;
	};

	RocketBullet(const RocketBulletData& data);
	virtual ~RocketBullet();

	//更新
	void Update() override;
	//描画
	void Draw() override;

	//ダメージが減らされるか
	bool IsDamageReduced() const override { return false; }
	//跳ね返したときの攻撃力を取得
	int GetReflectAttackPower() const override;

private:
	//プレイヤーを追跡する
	void HomingPlayer(const std::shared_ptr<EnemyBase> pTarget);

private:
	//弾からプレイヤーまでの撃った瞬間の距離
	float m_firstDistance = 0.0f;
	//ロケットの見た目
	std::unique_ptr<RocketModel> m_pModel;
};
