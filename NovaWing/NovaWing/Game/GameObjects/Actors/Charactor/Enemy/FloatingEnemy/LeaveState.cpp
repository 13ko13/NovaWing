#include "LeaveState.h"
#include "FloatingEnemy.h"
#include "Utility/Vector3.h"

namespace
{
	// 離脱速度
	constexpr float leave_speed = 7.0f;
	// どのくらい時間が経ったら死亡するか
	constexpr int leave_frame = 60 * 4;

	// 周期
	constexpr float wave_speed = 0.1f;
	// 速度
	constexpr float move_speed = 10.0f;
} // namespace

LeaveState::LeaveState(std::weak_ptr<FloatingEnemy> pEnemy)
	: IEnemyState(pEnemy)
{
}

LeaveState::~LeaveState()
{
}

void LeaveState::Enter()
{
	// 右か左に逃げていくような動きにする
	// GetRandは0を含むので-1する
	if (!m_isDecideDir)
	{
		m_leaveDir = static_cast<LeaveDirection>(GetRand(static_cast<int>(LeaveDirection::Max) - 1));
		m_isDecideDir = true;
	}
}

void LeaveState::Update()
{
	// フレームを更新
	m_frame++;
	// 速度を計算
	Vector3 vel; // 速度
	std::shared_ptr<FloatingEnemy> pEnemy = m_pEnemy.lock();
	
	switch (m_leaveDir)
	{
	case LeaveDirection::Right:
		vel = pEnemy->GetRight() * leave_speed;
		break;

	case LeaveDirection::Left:
		vel = pEnemy->GetLeft() * leave_speed;
		break;
	}
	
	// 上下に揺らす
	vel.m_y += sinf(m_frame * wave_speed) * move_speed;

	pEnemy->SetVel(vel);

	// TODO:画面外に行ったら死亡判定とする

	// 一応時間でも死亡するようにしておく
	if (m_frame >= leave_frame)
	{
		pEnemy->OnDead();
	}
}

void LeaveState::Exit()
{
}
