#include <EffekseerForDXLib.h>

#include "ChargeReadyState.h"
#include "Manager/InputManager.h"
#include "NormalShootState.h"
#include "Manager/BulletManager.h"
#include "Charactor/Player/Player.h"
#include "Manager/ResourceLoader.h"

namespace
{
	//チャージ弾を打てる許容時間
	constexpr int can_shoot_frame = 120;//2秒
	//弾の速度
	constexpr float move_speed = 25.0f;
	//攻撃力
	constexpr int attack_power = 100;
	//チャージ完了エフェクトを出す前後位置のオフセット
	constexpr float effect_offset_z = 200.0f;
}

ChargeReadyState::ChargeReadyState(const std::weak_ptr<Player> pPlayer,
	std::weak_ptr<BulletManager> pBulletManager) :
	IShootState(pPlayer, pBulletManager)
{

}

ChargeReadyState::~ChargeReadyState()
{
}

void ChargeReadyState::Exit()
{
	//エフェクトを停止する
	StopEffekseer3DEffect(m_chargingPlayEffectH);
}

void ChargeReadyState::Update()
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

	//ボタンが押されていればチャージ弾を発射
	if (input.IsTriggered(InputEvent::shoot))
	{
		//BulletManagerにチャージ弾発射を依頼する
		std::shared_ptr<BulletManager> pBulletManager = m_pBulletManager.lock();//一時的にshared_ptrに変換
		std::shared_ptr<Player> pPlayer = m_pPlayer.lock();//一時的にshared_ptrに変換
		const Vector3 pos = pPlayer->GetPos();//プレイヤーの位置
		const Vector3 vel = -pPlayer->GetForward() * move_speed;//速度

		//プレイヤーからターゲットを受け取る
		std::weak_ptr<GameObject> pTarget = m_pPlayer.lock()->GetFocusTarget();

		pBulletManager->CreateBullet(BulletManager::BulletType::ChargeBullet,
			pos, vel, attack_power, pTarget);

		//ノーマルステートに戻す
		ChangeState(std::make_shared<NormalShootState>(m_pPlayer, m_pBulletManager));
	}

#ifdef _DEBUG
	//チャージ完了デバッグ
	DrawFormatString(0, 350, 0xff0000, L"チャージ完了");
#endif // _DEBUG
}

void ChargeReadyState::Enter()
{
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();
	//プレイヤーからチャージ中のエフェクトの再生ハンドルを受け取る
	m_chargingPlayEffectH = pPlayer->GetChargingEffectHandle();

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
