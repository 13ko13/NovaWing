#include <EffekseerForDXLib.h>
#include "ReflectedBullet.h"

namespace
{
	//当たり判定球の半径
	constexpr float hit_col_radius = 32.0f;
}

ReflectedBullet::ReflectedBullet(ReflectBulletData& data) :
	BulletBase(data.pos,data.vel,data.attackPower,
		hit_col_radius,data.pCamera,ColliderTag::PlayerBullet),
	m_pTarget(data.pTarget),
	m_homingStrength(data.homingStrength)
{
	m_speed = data.vel.Length();

	//Effekseerのエフェクト再生を呼ぶ(見た目は敵弾のまま)
	m_effectPlayH = PlayEffekseer3DEffect(
		ResourceLoader::GetInstance().GetEffect(ResourceLoader::EffectID::EnemyBullet)
	);

	//再生直後に正しい位置へ即座にセットする(1フレーム目のワープ軌跡を防ぐ)
	SetPosPlayingEffekseer3DEffect(
		m_effectPlayH, data.pos.x, data.pos.y, data.pos.z
	);
}

ReflectedBullet::~ReflectedBullet()
{
	//エフェクトを止める
	StopEffekseer3DEffect(m_effectPlayH);
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
	SetPosPlayingEffekseer3DEffect(
		m_effectPlayH, GetPos().x, GetPos().y, GetPos().z
	);
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
	StopEffekseer3DEffect(m_effectPlayH);

	//消す処理
	OnDead();
}
