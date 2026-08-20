#include <EffekseerForDXLib.h>

#include "ChargeBullet.h"

namespace
{
	//ホーミング時の方向を変えるときのlerpの値
	constexpr float homing_lerp_t = 0.8f;
	//当たり判定の球の半径
	constexpr float radius = 32.0f;
}

ChargeBullet::ChargeBullet(const Vector3& pos, const Vector3& vel, int attackPower, std::weak_ptr<GameObject> pTarget):
	BulletBase(pos,vel,attackPower,radius),
	m_pTarget(pTarget)
{
	m_speed = vel.Length();

	//Effekseerのエフェクト再生を呼ぶ
	m_effectPlayHandle = PlayEffekseer3DEffect(
		ResourceLoader::GetInstance().GetEffect(ResourceLoader::EffectID::PlayerChargeBullet)
	);

	//再生直後に正しい位置へ即座にセットする(1フレーム目のワープ軌跡を防ぐ)
	SetPosPlayingEffekseer3DEffect(
		m_effectPlayHandle, pos.m_x, pos.m_y, pos.m_z
	);
}

ChargeBullet::~ChargeBullet()
{
	//エフェクトを止める
	StopEffekseer3DEffect(m_effectPlayHandle);
}

void ChargeBullet::Update()
{
	//基底クラスの更新処理
	BulletBase::Update();

	std::shared_ptr<GameObject> pTarget = m_pTarget.lock();

	//Nullじゃなければ、ターゲットが生きているかを取得して、
	//生きていればターゲットの方向に少しずつ向きを変える
	if (pTarget && !pTarget->IsDead())
	{
		//ターゲットの位置
		Vector3 targetPos = pTarget->GetPos();
		//ターゲットへの方向を計算
		Vector3 toTargetDir = targetPos - m_pos;
		toTargetDir.Normalize();//正規化

		//現在の進行方向と
		//ターゲットへの方向をlerpで少しずつ混ぜる
		Vector3 homingDir = Vector3::Lerp(m_velocity.Normalized(), toTargetDir, homing_lerp_t);
		homingDir.Normalize();

		//新しい進行方向を計算
		m_velocity = homingDir * m_speed;
	}

	//エフェクトの位置の調整する
	SetPosPlayingEffekseer3DEffect(
		m_effectPlayHandle, GetPos().m_x, GetPos().m_y, GetPos().m_z
	);
}

void ChargeBullet::Draw()
{
	//親クラスの描画処理
	BulletBase::Draw();
}

void ChargeBullet::OnHitEnemy()
{
	//弾の共通処理
	BulletBase::OnHitEnemy();

	//エフェクトを止める
	StopEffekseer3DEffect(m_effectPlayHandle);

	//消す処理
	OnDead();
}