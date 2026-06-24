#include "ChargeReadyState.h"
#include "Manager/InputManager.h"
#include "NormalShootState.h"

namespace
{
	//チャージ弾を打てる許容時間
	constexpr int can_shoot_frame = 60;//1秒
}

ChargeReadyState::ChargeReadyState(const std::weak_ptr<Player> pPlayer) :
	IShootState(pPlayer)
{
}

ChargeReadyState::~ChargeReadyState()
{
}

void ChargeReadyState::Exit()
{
}

void ChargeReadyState::Update()
{
	//チャージ完了してからの
	//発射待ちの間の時間を計測
	m_waitFrame++;

	InputManager& input = InputManager::GetInstance();

	//ボタンが押されて、発射待ちフレームが
	//許容範囲であればチャージ弾を発射
	if (input.IsTriggered("shoot") &&
		m_waitFrame < can_shoot_frame)
	{
		//TODO:BulletManagerにチャージ弾発射を依頼する

		//ノーマルステートに戻す
		ChangeState(std::make_shared<NormalShootState>(m_pPlayer));
	}
	//タイムアウトなら
	else if (m_waitFrame > can_shoot_frame)
	{
		//何もせずにノーマルステートに戻す
		ChangeState(std::make_shared<NormalShootState>(m_pPlayer));
	}
}

void ChargeReadyState::Enter()
{
	//待機フレームを初期化
	m_waitFrame = 0;
}
