#include "ChargeShootState.h"
#include "Manager/InputManager.h"
#include "NormalShootState.h"
#include "ChargeReadyState.h"
#include "Player.h"
#include "Manager/BulletManager.h"

namespace
{
	//チャージ完了と判定する秒数
	constexpr int charge_comp_frame = 60 * 2;//2秒
	//弾の速度
	constexpr float move_speed = 4.0f;
	//攻撃力
	constexpr int attack_power = 10;
}

ChargeShootState::ChargeShootState(const std::weak_ptr<Player> pPlayer,
	std::weak_ptr<BulletManager> pBulletManager):
	IShootState(pPlayer,pBulletManager)
{
}

ChargeShootState::~ChargeShootState()
{
}

void ChargeShootState::Exit()
{
}

void ChargeShootState::Update()
{
	InputManager& input = InputManager::GetInstance();

	//ボタンを押している間時間を計測
	if (input.IsPressed("shoot"))
	{
		m_chargeFrame++;
	}
	//もしボタンを離したときにチャージ未完了フレームだったら
	if (input.IsReleased("shoot") && 
		m_chargeFrame < charge_comp_frame)
	{
		//通常弾を発射して、ノーマルステートに戻す
		//BulletManagerに通常弾の発射を依頼する
		std::shared_ptr<BulletManager> pBulletManager = m_pBulletManager.lock();//一時的にshared_ptrに変換
		std::shared_ptr<Player> pPlayer = m_pPlayer.lock();//一時的にshared_ptrに変換
		const Vector3 pos = pPlayer->GetPos();//プレイヤーの位置
		const Vector3 vel = pPlayer->GetForward() * move_speed;//速度
		const ResourceLoader::ModelID id = ResourceLoader::ModelID::PlayerBullet;

		pBulletManager->CreateBullet(BulletManager::BulletType::PlayerBullet,
			pos, vel, attack_power, id);

		//ノーマルステートに戻す
		ChangeState(std::make_shared<NormalShootState>(m_pPlayer, m_pBulletManager));
	}
	//完了していたら
	else if(m_chargeFrame >= charge_comp_frame)
	{
		//チャージショット待機ステートに遷移する
		ChangeState(std::make_shared<ChargeReadyState>(m_pPlayer,m_pBulletManager));
	}
}

void ChargeShootState::Enter()
{
	//チャージフレームをリセットする
	m_chargeFrame = 0;
}
