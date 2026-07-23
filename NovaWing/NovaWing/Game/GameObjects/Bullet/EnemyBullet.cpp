#include <EffekseerForDXLib.h>

#include "EnemyBullet.h"

namespace
{
	//当たり判定の球の半径
	constexpr float radius = 32.0f;
}

EnemyBullet::EnemyBullet(const Vector3& pos, const Vector3& vel, const int attackPower):
	BulletBase(pos,vel,attackPower, radius)
{
	//Effekseerのエフェクト再生を呼ぶ
	m_effectPlayHandle = PlayEffekseer3DEffect(
		ResourceLoader::GetInstance().GetEffect(ResourceLoader::EffectID::EnemyBullet)
	);

	//再生直後に正しい位置へ即座にセットする(1フレーム目のワープ軌跡を防ぐ)
	SetPosPlayingEffekseer3DEffect(
		m_effectPlayHandle, pos.m_x, pos.m_y, pos.m_z
	);
}

EnemyBullet::~EnemyBullet()
{
	//エフェクトを止める
	StopEffekseer3DEffect(m_effectPlayHandle);
}

void EnemyBullet::Update()
{
	//親クラスの更新
	BulletBase::Update();

	//エフェクトの位置の調整する
	SetPosPlayingEffekseer3DEffect(
		m_effectPlayHandle, GetPos().m_x, GetPos().m_y, GetPos().m_z
	);
}

void EnemyBullet::Draw()
{
	//親クラスの描画処理
	BulletBase::Draw();
}

void EnemyBullet::OnHitEnemy()
{
	//エフェクトを止める
	StopEffekseer3DEffect(m_effectPlayHandle);

	//消す処理
	OnDead();
}
