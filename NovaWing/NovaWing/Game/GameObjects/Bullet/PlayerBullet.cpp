#include <EffekseerForDXLib.h>

#include "PlayerBullet.h"

namespace
{
	//当たり判定の球の半径
	constexpr float radius = 45.0f;
}

PlayerBullet::PlayerBullet(
	const Vector3& pos, const Vector3& vel,const int attackPower,
	std::weak_ptr<CameraBase> pCamera) :
	BulletBase(pos,vel,attackPower, radius,pCamera)
{
	//Effekseerのエフェクト再生を呼ぶ
	m_effectPlayHandle = PlayEffekseer3DEffect(
		ResourceLoader::GetInstance().GetEffect(ResourceLoader::EffectID::PlayerBullet)
	);

	//再生直後に正しい位置へ即座にセットする(1フレーム目のワープ軌跡を防ぐ)
	SetPosPlayingEffekseer3DEffect(
		m_effectPlayHandle, pos.m_x, pos.m_y, pos.m_z
	);
}

PlayerBullet::~PlayerBullet()
{
	//エフェクトを止める
	StopEffekseer3DEffect(m_effectPlayHandle);
}

void PlayerBullet::Update()
{
	//親クラスの更新処理
	BulletBase::Update();

	//エフェクトの位置の調整する
	SetPosPlayingEffekseer3DEffect(
		m_effectPlayHandle, GetPos().m_x, GetPos().m_y, GetPos().m_z
	);
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
	StopEffekseer3DEffect(m_effectPlayHandle);
	//消す処理
	OnDead();
}
