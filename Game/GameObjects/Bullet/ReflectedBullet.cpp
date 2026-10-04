#include "ReflectedBullet.h"
#include "Manager/EffectManager.h"

namespace
{
	//当たり判定球の半径
	constexpr float hit_col_radius = 32.0f;
}

ReflectedBullet::ReflectedBullet(
	const ReflectBulletData& data, std::weak_ptr<EffectManager> pEffectManager) :
	BulletBase(data.pos,data.vel,data.attackPower,
		hit_col_radius,data.pCamera,ColliderTag::PlayerBullet,pEffectManager),
	m_pTarget(data.pTarget),
	m_homingStrength(data.homingStrength)
{
	m_speed = data.vel.Length();

	//エフェクトの再生を依頼する(見た目は敵弾のまま)
	m_effectPlayH = m_pEffectManager.lock()->Play(ResourceLoader::EffectID::EnemyBullet, data.pos);
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

	std::shared_ptr<GameObject> pTarget = m_pTarget.lock();

	//Nullじゃなければ、ターゲットが生きているかを取得して、
	//生きていればターゲットの方向に少しずつ向きを変える
	if (pTarget && !pTarget->IsDead())
	{
		//ターゲットの位置
		Vector3 targetPos = pTarget->GetPos();
		//ターゲットへの方向を計算
		Vector3 toTargetDir = targetPos - m_pos;
		toTargetDir.Normalize();
		//ターゲットの方向へ少しずつ向かわせる
		Vector3 homingDir = Vector3::Lerp(m_velocity.Normalized(), toTargetDir, m_homingStrength);
		m_velocity = homingDir * m_speed; 
	}

	//エフェクトの位置の調整する
	m_pEffectManager.lock()->SetPos(m_effectPlayH, GetPos());
}

void ReflectedBullet::Draw()
{
	//親クラスの描画処理
	BulletBase::Draw();
}

void ReflectedBullet::OnHitEnemy()
{
	//弾の共通処理
	BulletBase::OnHitEnemy();

	//エフェクトを止める
	m_pEffectManager.lock()->Stop(m_effectPlayH);

	//消す処理
	OnDead();
}
