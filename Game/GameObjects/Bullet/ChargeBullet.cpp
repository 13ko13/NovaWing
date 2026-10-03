#include <EffekseerForDXLib.h>
#include <algorithm>

#include "ChargeBullet.h"

namespace
{
	//ホーミング時の方向を変えるときのlerpの値
	constexpr float homing_lerp_t = 0.8f;
	//当たり判定の球の半径
	constexpr float radius = 32.0f;

	//エフェクトの分身が集まり始めるターゲットまでの距離(弾速25なので約24フレーム前)
	constexpr float gather_start_dist = 600.0f;
	//分身が本体に重なりきる距離(敵の当たり半径+弾の半径くらい)
	constexpr float gather_end_dist = 160.0f;
	//分身の集まり具合を渡す動的パラメーターの番号
	constexpr int gather_input_index = 0;
}

ChargeBullet::ChargeBullet(
	const Vector3& pos,
	const Vector3& vel,
	int attackPower,
	std::weak_ptr<GameObject> pTarget,
	std::weak_ptr<CameraBase> pCamera):
	BulletBase(pos,vel,attackPower,radius,pCamera,ColliderTag::PlayerBullet),
	m_pTarget(pTarget)
{
	m_speed = vel.Length();

	//Effekseerのエフェクト再生を呼ぶ
	m_effectPlayHandle = PlayEffekseer3DEffect(
		ResourceLoader::GetInstance().GetEffect(ResourceLoader::EffectID::PlayerChargeBullet)
	);

	//再生直後に正しい位置へ即座にセットする(1フレーム目のワープ軌跡を防ぐ)
	SetPosPlayingEffekseer3DEffect(
		m_effectPlayHandle, pos.x, pos.y, pos.z
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

	//分身の集まり具合(0:散っている、1:本体に重なる)
	float gatherRate = 0.0f;

	//Nullじゃなければ、ターゲットが生きているかを取得して、
	//生きていればターゲットの方向に少しずつ向きを変える
	if (pTarget && !pTarget->IsDead())
	{
		//ターゲットの位置
		Vector3 targetPos = pTarget->GetPos();
		//ターゲットへの方向を計算
		Vector3 toTargetDir = targetPos - m_pos;

		//ターゲットに近づくほど分身を集める
		float toTargetDist = toTargetDir.Length();
		gatherRate = (gather_start_dist - toTargetDist) / (gather_start_dist - gather_end_dist);
		gatherRate = std::clamp(gatherRate, 0.0f, 1.0f);

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
		m_effectPlayHandle, GetPos().x, GetPos().y, GetPos().z
	);

	//エフェクトに分身の集まり具合を渡す
	SetDynamicInput3DEffect(m_effectPlayHandle, gather_input_index, gatherRate);
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