#include "PlayerBullet.h"

PlayerBullet::PlayerBullet(
	const Vector3& pos, const Vector3& vel,const int attackPower) :
	BulletBase(pos,vel,attackPower)
{
}

PlayerBullet::~PlayerBullet()
{
}

void PlayerBullet::Update()
{
	//親クラスの更新処理
	BulletBase::Update();
}

void PlayerBullet::Draw()
{
	//親クラスの描画処理
	BulletBase::Draw();
}

void PlayerBullet::OnHitEnemy()
{
	//消す処理
	OnDead();
}
