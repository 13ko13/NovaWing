#include <EffekseerForDXLib.h>
#include <algorithm>

#include "ChargeShootState.h"
#include "Manager/InputManager.h"
#include "NormalShootState.h"
#include "ChargeReadyState.h"
#include "Charactor/Player/Player.h"
#include "Manager/BulletManager.h"

namespace
{
	//チャージ完了と判定する秒数
	constexpr int charge_comp_frame = 60;//1秒
	//弾の速度
	constexpr float move_speed = 60.0f;
	//攻撃力
	constexpr int attack_power = 10;
	//チャージ完了エフェクトを出す前後位置のオフセット
	constexpr float effect_offset_z = 200.0f;

	//エフェクトの大きさを何フレームかけて小さくするか
	constexpr int change_eff_size_frame = 30;

	//エフェクトの最初の大きさ
	const Vector3 first_effect_scale = Vector3(1.0f, 1.0f, 1.0f);
}

ChargeShootState::ChargeShootState(const std::weak_ptr<Player> pPlayer,
	std::weak_ptr<BulletManager> pBulletManager) :
	IShootState(pPlayer, pBulletManager),
	m_effectScale(first_effect_scale)
{
}

ChargeShootState::~ChargeShootState()
{
}

void ChargeShootState::Exit()
{
	//もしチャージ未完了であればエフェクトは停止する
	if (m_chargeFrame < charge_comp_frame)
	{
		StopEffekseer3DEffect(m_chargingPlayEffectH);
	}
}

void ChargeShootState::Update()
{
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();

	//位置を設定(プレイヤーの位置)
	Vector3 playerPos = pPlayer->GetPos();
	//プレイヤーの前方向
	Vector3 playerForward = -pPlayer->GetForward();

	//エフェクトを出す位置
	Vector3 effectPos = playerPos + playerForward * effect_offset_z;

	//エフェクトの位置をプレイヤーの位置に設定する
	SetPosPlayingEffekseer3DEffect(
		m_chargingPlayEffectH,
		effectPos.m_x,
		effectPos.m_y,
		effectPos.m_z);


	InputManager& input = InputManager::GetInstance();

	//ボタンを押している間時間を計測
	if (input.IsPressed(InputEvent::shoot))
	{
		m_chargeFrame++;
	}
	else
	{
		//全方向の大きさを下げる
		m_effectScale.m_x--;
		m_effectScale.m_y--;
		m_effectScale.m_z--;
		//0未満にならないようにクランプする
		m_effectScale.m_x = std::clamp(m_effectScale.m_x, 0.0f, m_effectScale.m_x);
		m_effectScale.m_y = std::clamp(m_effectScale.m_y, 0.0f, m_effectScale.m_y);
		m_effectScale.m_z = std::clamp(m_effectScale.m_z, 0.0f, m_effectScale.m_z);

		//大きさをセットする
		SetScalePlayingEffekseer3DEffect(
			m_chargingPlayEffectH,
			m_effectScale.m_x,
			m_effectScale.m_y,
			m_effectScale.m_z
		);

		//もし大きさが0になったらノーマルステートに戻す
		if (m_effectScale.m_x == 0.0f)
		{
			//ノーマルステートに戻す
			ChangeState(std::make_shared<NormalShootState>(m_pPlayer, m_pBulletManager));
		}
	}

	//もしボタンを離したときにチャージ未完了フレームだったら
	if (input.IsReleased(InputEvent::shoot) &&
		m_chargeFrame < charge_comp_frame)
	{
		//通常弾を発射して、ノーマルステートに戻す
		//BulletManagerに通常弾の発射を依頼する
		std::shared_ptr<BulletManager> pBulletManager = m_pBulletManager.lock();//一時的にshared_ptrに変換
		std::shared_ptr<Player> pPlayer = m_pPlayer.lock();//一時的にshared_ptrに変換
		const Vector3 pos = pPlayer->GetPos();//プレイヤーの位置
		const Vector3 vel = -pPlayer->GetForward() * move_speed;//速度

		pBulletManager->CreateBullet(BulletManager::BulletType::PlayerBullet,
			pos, vel, attack_power);
	}
	//完了していたら
	else if (m_chargeFrame >= charge_comp_frame)
	{
		//チャージショット待機ステートに遷移する
		ChangeState(std::make_shared<ChargeReadyState>(m_pPlayer, m_pBulletManager));
	}
}

void ChargeShootState::Enter()
{
	//チャージフレームをリセットする
	m_chargeFrame = 0;

	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();

	//チャージ中のエフェクトの再生を行い、そのハンドルをプレイヤーに渡す
	m_chargingPlayEffectH = PlayEffekseer3DEffect(
		ResourceLoader::GetInstance().GetEffect(
			ResourceLoader::EffectID::Charging));

	pPlayer->SetChargingEffectHandle(m_chargingPlayEffectH);

	//位置を設定(プレイヤーの位置)
	Vector3 playerPos = pPlayer->GetPos();
	//プレイヤーの前方向
	Vector3 playerForward = -pPlayer->GetForward();

	//エフェクトを出す位置
	Vector3 effectPos = playerPos + playerForward * effect_offset_z;

	//エフェクトの位置をプレイヤーの位置に設定する
	SetPosPlayingEffekseer3DEffect(
		m_chargingPlayEffectH,
		effectPos.m_x,
		effectPos.m_y,
		effectPos.m_z);
}
