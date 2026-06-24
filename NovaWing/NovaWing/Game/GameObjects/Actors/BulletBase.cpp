#include "BulletBase.h"

namespace
{
	//寿命
	constexpr int life_time = 60;//1秒
	//球のサイズ
	constexpr float sphere_size = 16.0f;
}

BulletBase::BulletBase(const Vector3& pos, const Vector3& vel, int attackPower,
	ResourceLoader::ModelID modelId):
	Actor(modelId),
	m_sphere(pos),
	m_attackPower(attackPower),
	m_lifeTime(0)
{
	//位置を設定
	SetPos(pos);
	//速度を設定
	SetVel(vel);
}

BulletBase::~BulletBase()
{

}

void BulletBase::Update()
{
	//位置を更新
	m_pos += m_velocity;

	//オブジェクトの寿命を更新
	m_lifeTime++;
	//寿命を越えたら死亡判定にする
	if (m_lifeTime > life_time)
	{
		OnDead();
	}
	//TODO:画面外判定を決めてそこを越えたら死亡判定にする

	//球の位置を更新
	m_sphere.Update(m_pos, sphere_size);
}

void BulletBase::Draw()
{
	//モデルの描画
	MV1DrawModel(m_modelHandle);
}
