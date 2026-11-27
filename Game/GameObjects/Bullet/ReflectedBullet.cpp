#include "ReflectedBullet.h"
#include "Manager/EffectManager.h"
#include "Game/GameObjects/Actors/Character/Enemy/EnemyBase.h"

namespace
{
	//当たり判定球の半径
	constexpr float hit_col_radius = 32.0f;
}

ReflectedBullet::ReflectedBullet(
	const ReflectBulletData& data, std::weak_ptr<EffectManager> pEffectManager) :
	BulletBase(data.pos, data.vel, data.attackPower,
		hit_col_radius, data.pCamera, ColliderTag::PlayerBullet, pEffectManager),
	m_pTarget(data.pTarget),
	m_homingStrength(data.homingStrength),
	m_isDamageReduced(data.isDamageReduced)
{
	m_speed = data.vel.Length();

	//敵の弾がモデルなしだった場合はエフェクトを再生
	if (data.pModel == nullptr)
	{
		//エフェクトの再生を依頼する(見た目は敵弾のまま)
		m_effectPlayH = m_pEffectManager.lock()->Play(
			ResourceLoader::EffectID::EnemyBullet, data.pos);
	}
	//敵の弾がモデルありだった場合モデルを受け取る
	else
	{
		//モデルを受け取る
		m_pModel = data.pModel;
	}
}

ReflectedBullet::~ReflectedBullet()
{
	//エフェクトを止める
	//(シーン終了でマネージャーが先に消えている場合は止める必要がない)
	if (std::shared_ptr<EffectManager> pEffectManager = m_pEffectManager.lock())
	{
		pEffectManager->Stop(m_effectPlayH);
	}
}

void ReflectedBullet::Update()
{
	//基底クラスの更新処理
	BulletBase::Update();

	std::shared_ptr<EnemyBase> pTarget = m_pTarget.lock();

	//Nullじゃなければ、ターゲットが生きているかを取得して、
	//生きていればターゲットの方向に少しずつ向きを変える
	if (pTarget && !pTarget->IsDead())
	{
		//ターゲットの位置
		Vector3 targetPos = pTarget->GetLockablePos();
		//ターゲットへの方向を計算
		Vector3 toTargetDir = targetPos - m_pos;
		toTargetDir.Normalize();
		//ターゲットの方向へ少しずつ向かわせる
		Vector3 homingDir = Vector3::Lerp(m_velocity.Normalized(), toTargetDir, m_homingStrength);
		m_velocity = homingDir * m_speed;
	}

	//モデルがないときだけエフェクトの位置を調整する
	if (!m_pModel)
	{
		//エフェクトの位置の調整する
		m_pEffectManager.lock()->SetPos(m_effectPlayH, GetPos());
	}
}

void ReflectedBullet::Draw()
{
	//親クラスの描画処理
	BulletBase::Draw();

	//モデルがある場合だけSetPos,LookAtDir,Drawを呼ぶ
	if(m_pModel)
	{
		SetPos(m_pos);
		// LookAtDir(m_velocity);
		Draw();
	}
}

void ReflectedBullet::OnHitEnemy(ColliderTag hitTag)
{
	//弾の共通処理
	BulletBase::OnHitEnemy(hitTag);

	//エフェクトを止める
	m_pEffectManager.lock()->Stop(m_effectPlayH);

	//消す処理
	OnDead();
}
