#include <EffekseerForDXLib.h>

#include "BulletBase.h"
#include "Manager/LightingManager.h"
#include "Game/GameObjects/Camera/CameraBase.h"

namespace
{
	//寿命
	constexpr int life_time = 300;//3秒
	//モデルのサイズ
	const Vector3 model_size = Vector3(0.4f, 0.4f, 0.4f);
}

BulletBase::BulletBase(
	const Vector3& pos,
	const Vector3& vel,
	int attackPower,
	float radius,
	std::weak_ptr<CameraBase> pCamera):
	m_attackPower(attackPower),
	m_lifeTime(0),
	m_radius(radius),
	m_pCamera(pCamera)
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

	//カメラのfarを超えたら死亡にする
	float far_clip = m_pCamera.lock()->GetFarClip();
	//カメラと弾の間のベクトルの距離がfar_clipを超えたら
	Vector3 cameraToBullet = m_pos - m_pCamera.lock()->GetPos();
	float cameraToBulletDistance = cameraToBullet.Length();

	if (cameraToBulletDistance > far_clip)
	{
		//消す
		OnDead();
	}

	//球の位置を更新
	m_sphere.Update(m_pos, m_radius);
}

void BulletBase::Draw()
{
#ifdef _DEBUG
	//球を描画
	m_sphere.Draw(0xffffff);
#endif
}

void BulletBase::OnHitEnemy()
{
	//ヒットエフェクトは共通なのでここで行う
	int hitEffectHandle = ResourceLoader::GetInstance().GetEffect(
		ResourceLoader::EffectID::HitEffect
	);
	m_hitEffectPlayH = PlayEffekseer3DEffect(hitEffectHandle);
	
	//位置をセット
	SetPosPlayingEffekseer3DEffect(m_hitEffectPlayH, m_pos.m_x, m_pos.m_y, m_pos.m_z);
}
