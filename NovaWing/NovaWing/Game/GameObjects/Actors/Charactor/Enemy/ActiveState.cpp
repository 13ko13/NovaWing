#include <cmath>

#include "ActiveState.h"
#include "Utility/Vector3.h"
#include "FloatingEnemy.h"
#include "LeaveState.h"

namespace
{
	//速度
	constexpr float move_speed =6.0f;
	//周期
	constexpr float wave_speed = 0.05f;
	//ActiveStateを何フレーム続けるか
	constexpr int active_frame = 60 * 3;
}

ActiveState::ActiveState(std::weak_ptr<FloatingEnemy> pEnemy):
	IEnemyState(pEnemy)
{

}

ActiveState::~ActiveState()
{

}

void ActiveState::Enter()
{

}

void ActiveState::Update()
{
	//フレームを更新
	m_frame++;

	//上下の揺れ
	Vector3 vel;
	vel.m_y = sinf(m_frame * wave_speed) * move_speed;
	//プレイヤーの前方向と同じ方向に一緒に進む
	std::shared_ptr<FloatingEnemy> pEnemy = m_pEnemy.lock();
	Vector3 forward = pEnemy->GetPlayerFoward();
	vel += -forward * move_speed;

	//敵の速度に適用する
	
	pEnemy->SetVel(vel);
#ifdef _DEBUG
	DrawFormatString(0, 230, 0xffffff, L"velY : %f", vel.m_y);
#endif

	//TODO:弾の発射
	//一定周期で

	//Activeの時間が終了したら戦闘離脱ステートに遷移
	if (m_frame > active_frame)
	{
		//遷移
		ChangeState(std::make_shared<LeaveState>(pEnemy));
	}

}

void ActiveState::Exit()
{

}