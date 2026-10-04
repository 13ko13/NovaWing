#include "PlayerBullet.h"
#include "Manager/EffectManager.h"

namespace
{
	//当たり判定の球の半径
	constexpr float radius = 45.0f;
}

PlayerBullet::PlayerBullet(
	const Vector3& pos, const Vector3& vel,const int attackPower,
	std::weak_ptr<CameraBase> pCamera,
	std::weak_ptr<EffectManager> pEffectManager) :
	BulletBase(pos,vel,attackPower, radius,pCamera,ColliderTag::PlayerBullet,pEffectManager)
{
	//エフェクトの再生を依頼する
	m_effectPlayHandle = m_pEffectManager.lock()->Play(ResourceLoader::EffectID::PlayerBullet, pos);
}

PlayerBullet::~PlayerBullet()
{
	//エフェクトを止める
	//(シーン終了でマネージャーが先に消えている場合は止める必要がない)
	if (std::shared_ptr<EffectManager> pEffectManager = m_pEffectManager.lock())
	{
		pEffectManager->Stop(m_effectPlayHandle);
	}
}

void PlayerBullet::Update()
{
	//親クラスの更新処理
	BulletBase::Update();

	//エフェクトの位置の調整する
	m_pEffectManager.lock()->SetPos(m_effectPlayHandle, GetPos());
}

void PlayerBullet::Draw()
{
	//親クラスの描画処理
	BulletBase::Draw();
}

void PlayerBullet::OnHitEnemy()
{
	//弾の共通処理
	BulletBase::OnHitEnemy();

	//エフェクトを止める
	m_pEffectManager.lock()->Stop(m_effectPlayHandle);
	//消す処理
	OnDead();
}
