#define NOMINMAX
#include <DxLib.h>
#include <cmath>

#include "Charactor/Player/Player.h"
#include "Manager/SoundManager.h"
#include "NoneState.h"
#include "SomersaultState.h"

namespace
{
	// 宙返りをどれぐらいの時間をかけて行うか
	constexpr float max_frame = 160.0f;
	// 移動する際の速度
	constexpr float move_speed = 20.0f;
	// ゲージ消費量
	constexpr float gauge_consumption = -0.3f;
	// 宙返り開始時にY回転を180度へ寄せるLerpのt値
	constexpr float enter_rot_lerp_t = 0.9f;
} // namespace

SomersaultState::SomersaultState(const std::weak_ptr<Player> pPlayer,
	std::weak_ptr<SoundManager> pSoundManager) :
	ISpecialActionState(pPlayer),
	m_pSoundManager(pSoundManager)
{
}

SomersaultState::~SomersaultState()
{
}

void SomersaultState::Enter()
{
	// ゲージ使用を開始したことをプレイヤーに伝える
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();
	pPlayer->StartUseGauge();

	m_frame = 0;
	// プレイヤーの左右回転をリセット
	pPlayer->LerpToAngleY(DX_PI_F, enter_rot_lerp_t);

	// 宙返り音を鳴らす
	m_pSoundManager.lock()->Play(SoundManager::SoundType::Somersoult);
}

void SomersaultState::Update()
{
	// フレームを更新
	m_frame++;

	// 進行度を計算
	float progress = static_cast<float>(m_frame) / max_frame;
	// Xの回転角を進行度に合わせて求める
	float targetAngleX = progress * DX_TWO_PI_F; // 0～360度

	// プレイヤーに回転を適用
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();
	pPlayer->LerpToAngleX(-targetAngleX, 1.0f); // 補完は行わない

	// ゲージを消費
	pPlayer->ChangeGauge(gauge_consumption);

	// 速度
	Vector3 vel;
	vel.m_y = sinf(targetAngleX) * move_speed;
	vel.m_z = cosf(-targetAngleX) * move_speed;
	// 速度を設定
	pPlayer->SetVel(vel);

	// 宙返りの時間が終了したらステートを戻す
	if (m_frame >= max_frame)
	{
		ChangeState(std::make_shared<NoneState>(pPlayer));
	}
}

void SomersaultState::Exit()
{
	// プレイヤーの回転を0にする
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();
	pPlayer->LerpToAngleX(0.0f, 1.0f);

	// ゲージ使用を中止したことをプレイヤーに伝える
	pPlayer->EndUseGauge();
}