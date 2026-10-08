#include <algorithm>

#include "ChargeBullet.h"
#include "Manager/EffectManager.h"
#include "Game/Collision/ChargeExplosionCollider.h"
#include "Manager/DebugManager.h"

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

	//爆発範囲
	constexpr float explosion_radius = 700.0f;

	//爆発の持続フレーム
	constexpr int explosion_max_frame = 60;
}

ChargeBullet::ChargeBullet(
	const Vector3& pos,
	const Vector3& vel,
	int attackPower,
	std::weak_ptr<GameObject> pTarget,
	std::weak_ptr<CameraBase> pCamera,
	std::weak_ptr<EffectManager> pEffectManager):
	BulletBase(pos,vel,attackPower,radius,pCamera,ColliderTag::ChargeBullet,pEffectManager),
	m_pTarget(pTarget)
{
	m_speed = vel.Length();

	//エフェクトの再生を依頼する
	m_effectPlayHandle = m_pEffectManager.lock()->Play(
		ResourceLoader::EffectID::PlayerChargeBullet, pos);

	//爆発用の球を作成
	m_pExplosionSphere = std::make_shared<SphereShape>(m_pos, explosion_radius);

	//コライダーのユニークポインタを作成
	m_pExplosionCollider = std::make_unique<ChargeExplosionCollider>(*this);
}

ChargeBullet::~ChargeBullet()
{
	//エフェクトを止める
	//(シーン終了でマネージャーが先に消えている場合は止める必要がない)
	if (std::shared_ptr<EffectManager> pEffectManager = m_pEffectManager.lock())
	{
		pEffectManager->Stop(m_effectPlayHandle);
	}
}

void ChargeBullet::Update()
{
	//爆発してない間のみ行う
	if(!m_isExploding)
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
		std::shared_ptr<EffectManager> pEffectManager = m_pEffectManager.lock();
		pEffectManager->SetPos(m_effectPlayHandle, GetPos());

		//エフェクトに分身の集まり具合を渡す
		pEffectManager->SetDynamicInput(m_effectPlayHandle, gather_input_index, gatherRate);
	}
	//爆発中の処理
	else
	{
		//爆発更新フレーム更新	
		m_explosionFrame++;
		if (m_explosionFrame > explosion_max_frame)
		{
			//爆発が終了したら死ぬ
			OnDead();
		}
	}
}

void ChargeBullet::Draw()
{
	//親クラスの描画処理
	BulletBase::Draw();

#ifdef _DEBUG
	//爆発中の描画
	if (DebugManager::GetInstance().IsDebugDrawEnabled() && m_isExploding)
	{
		m_pExplosionSphere->Draw(0xffffff);
	}
#endif
}

void ChargeBullet::OnHitEnemy(ColliderTag hitTag)
{
	//弾の共通処理
	BulletBase::OnHitEnemy(hitTag);

	//エフェクトを止める
	m_pEffectManager.lock()->Stop(m_effectPlayHandle);

	//ボスに当たった場合はエフェクトを起こさ
	if (hitTag == ColliderTag::BossDamage ||
		hitTag == ColliderTag::BossShield)
	{
		//弾も消す
		OnDead();
	}
	else
	{
		//OnDeadを呼ばずに、爆発開始する
		m_isExploding = true;
		m_explosionFrame = 0;
		//1フレームだけ爆発の球を更新する
		m_pExplosionSphere->Update(m_pos, explosion_radius);
		//爆発エフェクトを出す
		m_pEffectManager.lock()->PlayOneShot(ResourceLoader::EffectID::ChargeExplosion, m_pos);	
	}
}

ICollider& ChargeBullet::GetExplosionCollider()
{
	//爆発用のコライダーを返す
	return *m_pExplosionCollider;
}