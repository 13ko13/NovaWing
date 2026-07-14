#pragma once
#include "BulletBase.h"

class PlayerBullet : public BulletBase
{
public:
	PlayerBullet(const Vector3& pos,const Vector3& vel,
		const int attackPower);
	~PlayerBullet();

	void Update() override;//更新処理
	void Draw() override;//描画処理

	//弾が敵に当たった時の処理
	void OnHitEnemy() override;

private:
//Effekseerのエフェクト再生中のハンドル
	int m_effectPlayHandle = -1;
};

