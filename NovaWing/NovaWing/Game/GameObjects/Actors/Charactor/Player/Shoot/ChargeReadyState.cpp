#include <EffekseerForDXLib.h>
#include <algorithm>

#include "Charactor/Player/Player.h"
#include "ChargeReadyState.h"
#include "Manager/BulletManager.h"
#include "Manager/InputManager.h"
#include "Manager/ResourceLoader.h"
#include "Manager/SoundManager.h"
#include "NormalShootState.h"

namespace
{
	// チャージ弾を打てる許容時間
	constexpr int can_shoot_frame = 60; // 2秒
	// 弾の速度
	constexpr float move_speed = 25.0f;
	// 攻撃力
	constexpr int attack_power = 100;
	// チャージ完了エフェクトを出す前後位置のオフセット
	constexpr float effect_offset_z = 200.0f;

	// エフェクトの最初の大きさ
	const Vector3 first_effect_scale = Vector3(1.0f, 1.0f, 1.0f);
} // namespace

ChargeReadyState::ChargeReadyState(
	const std::weak_ptr<Player> pPlayer,
	std::weak_ptr<BulletManager> pBulletManager,
	std::weak_ptr<SoundManager> pSoundManager,
	std::weak_ptr<TargetManager> pTargetManager) : 
	IShootState(pPlayer, pBulletManager, pSoundManager,pTargetManager),
	m_effectScale(first_effect_scale)
{
}

ChargeReadyState::~ChargeReadyState()
{
}

void ChargeReadyState::Exit()
{
	// エフェクトを停止する
	StopEffekseer3DEffect(m_chargingPlayEffectH);
}

void ChargeReadyState::Update()
{
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();

	// 位置を設定(プレイヤーの位置)
	Vector3 playerPos = pPlayer->GetPos();
	// プレイヤーの前方向
	Vector3 playerForward = -pPlayer->GetForward();

	// エフェクトを出す位置
	Vector3 effectPos = playerPos + playerForward * effect_offset_z;

	// エフェクトの位置をプレイヤーの位置に設定する
	SetPosPlayingEffekseer3DEffect(
		m_chargingPlayEffectH,
		effectPos.m_x,
		effectPos.m_y,
		effectPos.m_z);

	InputManager& input = InputManager::GetInstance();

	// ボタンが離されていて、時間内にボタンが押されていなければ
	// 弾を打たずに通常状態に戻る
	if (!input.IsPressed(InputEvent::shoot))
	{
		// 離されている間の時間を計測
		m_notPressdFrame++;

		// エフェクトを小さくするフラグを立てる
		m_canShrink = true;
	}
	if (m_notPressdFrame < can_shoot_frame)
	{
		// 時間内に弾を打った場合
		if (input.IsTriggered(InputEvent::shoot))
		{
			// BulletManagerにチャージ弾発射を依頼する
			std::shared_ptr<BulletManager> pBulletManager = m_pBulletManager.lock(); // 一時的にshared_ptrに変換
			std::shared_ptr<Player> pPlayer = m_pPlayer.lock();						 // 一時的にshared_ptrに変換
			const Vector3 pos = pPlayer->GetPos();									 // プレイヤーの位置
			const Vector3 vel = -pPlayer->GetForward() * move_speed;				 // 速度

			// プレイヤーからターゲットを受け取る
			std::weak_ptr<EnemyBase> pTarget = m_pPlayer.lock()->GetForcusTarget();

			pBulletManager->CreateBullet(BulletManager::BulletType::ChargeBullet,
										 pos, vel, attack_power, pTarget);

			// チャージショット発射音を鳴らす
			m_pSoundManager.lock()->Play(SoundManager::SoundType::ChargeShoot);
		}
	}

	// エフェクトを小さくしていいフラグがたっていれば小さくする
	// エフェクトの大きさが0になればノーマルステートに戻す
	if (m_canShrink)
	{
		// 全方向の大きさを下げる
		m_effectScale.m_x -= 0.07f;
		m_effectScale.m_y -= 0.07f;
		m_effectScale.m_z -= 0.07f;
		// 0未満にならないようにクランプする
		m_effectScale.m_x = std::clamp(m_effectScale.m_x, 0.0f, first_effect_scale.m_x);
		m_effectScale.m_y = std::clamp(m_effectScale.m_y, 0.0f, first_effect_scale.m_y);
		m_effectScale.m_z = std::clamp(m_effectScale.m_z, 0.0f, first_effect_scale.m_z);

		// 大きさをセットする
		SetScalePlayingEffekseer3DEffect(
			m_chargingPlayEffectH,
			m_effectScale.m_x,
			m_effectScale.m_y,
			m_effectScale.m_z);

		// もし大きさが0になったらノーマルステートに戻す
		if (m_effectScale.m_x == 0.0f)
		{
			// ノーマルステートに戻す
			ChangeState(std::make_shared<NormalShootState>(m_pPlayer, m_pBulletManager, m_pSoundManager,m_pTargetManager));
		}
	}
}

void ChargeReadyState::Enter()
{
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();
	// プレイヤーからチャージ中のエフェクトの再生ハンドルを受け取る
	m_chargingPlayEffectH = pPlayer->GetChargingEffectHandle();

	// 位置を設定(プレイヤーの位置)
	Vector3 playerPos = pPlayer->GetPos();
	// プレイヤーの前方向
	Vector3 playerForward = -pPlayer->GetForward();

	// エフェクトを出す位置
	Vector3 effectPos = playerPos + playerForward * effect_offset_z;

	// エフェクトの位置をプレイヤーの位置に設定する
	SetPosPlayingEffekseer3DEffect(
		m_chargingPlayEffectH,
		effectPos.m_x,
		effectPos.m_y,
		effectPos.m_z);
}
