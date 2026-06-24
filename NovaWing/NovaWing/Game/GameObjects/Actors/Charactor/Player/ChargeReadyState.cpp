#include "ChargeReadyState.h"

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

	//ボタンが押されて、発射待ちフレームが
	//許容範囲であればチャージ弾を発射
	if (m_waitFrame < can_shoot_frame)
	{
		//TODO:BulletManagerにチャージ弾発射を依頼する

	}
}

void ChargeReadyState::Enter()
{
}
