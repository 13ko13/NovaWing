#include <EffekseerForDXLib.h>

#include "PlayerBullet.h"

PlayerBullet::PlayerBullet(
	const Vector3& pos, const Vector3& vel,const int attackPower) :
	BulletBase(pos,vel,attackPower)
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
	//エフェクトを止める
	StopEffekseer3DEffect(m_effectPlayHandle);
	//消す処理
	OnDead();
}
