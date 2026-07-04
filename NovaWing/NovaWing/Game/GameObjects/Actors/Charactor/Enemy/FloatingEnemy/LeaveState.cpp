#include "LeaveState.h"
#include "Utility/Vector3.h"
#include "FloatingEnemy.h"

namespace
{
	//離脱速度
	constexpr float leave_speed = 7.0f;
	//どのくらい時間が経ったら死亡するか
	constexpr int leave_frame = 60 * 4;

	//周期
	constexpr float wave_speed = 0.1f;
	//速度
	constexpr float move_speed =10.0f;
}

LeaveState::LeaveState(std::weak_ptr<FloatingEnemy> pEnemy):
	IEnemyState(pEnemy)
{

}

LeaveState::~LeaveState()
{

}

void LeaveState::Enter()
{
	
}

void LeaveState::Update()
{
	//フレームを更新
	m_frame++;
	//速度を計算
	//右に向かって上下に揺れながら逃げていくような動きをする
	Vector3 vel;//速度
	std::shared_ptr<FloatingEnemy> pEnemy = m_pEnemy.lock();
	//右にleave_speedの速度で逃げる
	vel = pEnemy->GetRight() * leave_speed;
	//上下に揺らす
	vel.m_y += sinf(m_frame * wave_speed) * move_speed;

	pEnemy->SetVel(vel);

	//TODO:画面外に行ったら死亡判定とする

	//一応時間でも死亡するようにしておく
	if (m_frame >= leave_frame)
	{
		pEnemy->OnDead();
	}
}

void LeaveState::Exit()
{

}
