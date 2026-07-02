#pragma once
#include "BulletBase.h"
class EnemyBullet : public BulletBase
{
public:
	EnemyBullet(const Vector3& pos, const Vector3& vel,
		const int attackPower);
	~EnemyBullet();

	void Update() override;//更新処理
	void Draw() override;//描画処理

	//弾がプレイヤーに当たった時の処理
	void OnHitPlayer();
};

