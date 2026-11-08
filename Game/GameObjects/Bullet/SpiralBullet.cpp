#include <EffekseerForDXLib.h>

#include "SpiralBullet.h"
#include "Manager/EffectManager.h"
#include "Main/Application.h"

namespace
{
	//回転する弾を軸からどれぐらい離すか
	constexpr float spiral_distance = 300.0f;
	//1フレームで何周するか
	constexpr float rotation_per_frame = DX_TWO_PI_F / 60.0f;
}

SpiralBullet::SpiralBullet(const Vector3& pos, const Vector3& vel,
		const int attackPower, std::weak_ptr<CameraBase> pCamera,
		std::shared_ptr<EnemyBase> pShooter,
		std::weak_ptr<EffectManager> pEffectManager,
		float startAngle) :
	EnemyBullet(pos, vel, attackPower, pCamera, pShooter, pEffectManager),
	m_bulletAngle(startAngle)
{
}

SpiralBullet::~SpiralBullet()
{
}

void SpiralBullet::Update()
{
	//弾の共通更新処理
	EnemyBullet::Update();
	//タイムスケール
	float timeScale = Application::GetInstance().GetTimeScale();

	//毎フレーム角度を進める
	m_bulletAngle += rotation_per_frame * timeScale;

	//軸上をspiral_distance分離れた場所で回る
	m_spiralPos = Vector3(
		m_pos.x + std::cosf(m_bulletAngle) * spiral_distance,
		m_pos.y + std::sinf(m_bulletAngle) * spiral_distance,
		m_pos.z
	);

	//当たり判定の位置は回転する弾の位置に合わせる
	m_sphere->Update(m_spiralPos, m_radius);
	//エフェクトも回転する弾の位置に合わせる
	m_pEffectManager.lock()->SetPos(m_effectPlayHandle, m_spiralPos);
}
