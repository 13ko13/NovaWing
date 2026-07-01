#include "EnemyBullet.h"

EnemyBullet::EnemyBullet(const Vector3& pos, const Vector3& vel, const int attackPower):
	BulletBase(pos,vel,attackPower)
{
}

EnemyBullet::~EnemyBullet()
{
}

void EnemyBullet::Update()
{
	//親クラスの更新
	BulletBase::Update();
}

void EnemyBullet::Draw()
{
	//親クラスの描画処理
	BulletBase::Draw();
}

void EnemyBullet::OnHitPlayer()
{
	//消す処理
	OnDead();
}
