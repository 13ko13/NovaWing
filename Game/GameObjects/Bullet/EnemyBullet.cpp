#include "EnemyBullet.h"
#include "Manager/EffectManager.h"
#include "Constants/Game.h"

namespace
{
	//当たり判定の球の半径
	constexpr float radius = 32.0f;
}

EnemyBullet::EnemyBullet(
	const Vector3& pos, const Vector3& vel,
	const int attackPower, std::weak_ptr<CameraBase> pCamera,
	std::shared_ptr<EnemyBase> pShooter,
	std::weak_ptr<EffectManager> pEffectManager):
	BulletBase(pos,vel,attackPower, radius,pCamera,ColliderTag::EnemyBullet,pEffectManager),
	m_pShooter(pShooter)
{
	//エフェクトの再生を依頼する
	m_effectPlayHandle = m_pEffectManager.lock()->Play(ResourceLoader::EffectID::EnemyBullet, pos);
}

EnemyBullet::~EnemyBullet()
{
	//エフェクトを止める
	//(シーン終了でマネージャーが先に消えている場合は止める必要がない)
	if (std::shared_ptr<EffectManager> pEffectManager = m_pEffectManager.lock())
	{
		pEffectManager->Stop(m_effectPlayHandle);
	}
}

void EnemyBullet::Update()
{
	//親クラスの更新
	BulletBase::Update();

	//エフェクトの位置の調整する
	std::shared_ptr<EffectManager> pEffectManager = m_pEffectManager.lock();
	pEffectManager->SetPos(m_effectPlayHandle, GetPos());
	//
	pEffectManager->SetDynamicInput(m_effectPlayHandle, 0, GetPos().y - 50.0f);

	//自身がカメラの視野範囲内に入っていなければ消す
	Vector3 halfSize = Vector3(m_radius, m_radius, m_radius);
	Vector3 boxMin = m_pos - halfSize;
	Vector3 boxMax = m_pos + halfSize;
	if (CheckCameraViewClip_Box(boxMin.ToDxLib(), boxMax.ToDxLib()))
	{
		OnDead();
	}
}

void EnemyBullet::Draw()
{
	//親クラスの描画処理
	BulletBase::Draw();
}

void EnemyBullet::OnHitEnemy(ColliderTag hitTag)
{
	//弾共通処理
	BulletBase::OnHitEnemy(hitTag);

	//エフェクトを止める
	m_pEffectManager.lock()->Stop(m_effectPlayHandle);

	//消す処理
	OnDead();
}
