#include "ChargeShootState.h"
#include "Manager/InputManager.h"
#include "NormalShootState.h"

namespace
{
	//チャージ完了と判定する秒数
	constexpr int charge_comp_frame = 60 * 2;//2秒
}

ChargeShootState::ChargeShootState(const std::weak_ptr<Player> pPlayer):
	IShootState(pPlayer)
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
		//TODO:BulletManagerに通常弾の発射を依頼する

		//ノーマルステートに戻す
		ChangeState(std::make_shared<NormalShootState>(m_pPlayer));
	}
	//完了していたら
	else if(m_chargeFrame >= charge_comp_frame)
	{
		//TODO:チャージショット待機ステートに遷移する

	}
}

void ChargeShootState::Enter()
{
	//チャージフレームをリセットする
	m_chargeFrame = 0;
}
