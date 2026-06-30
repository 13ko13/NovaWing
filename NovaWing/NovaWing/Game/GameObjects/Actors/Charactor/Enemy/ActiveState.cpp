#include <cmath>

#include "ActiveState.h"
#include "Utility/Vector3.h"
#include "FloatingEnemy.h"

namespace
{
	//フレーム数を更新する周期
	constexpr int max_frame = 60;
	//振幅
	constexpr float move_amplitude =4.0f;
}

ActiveState::ActiveState(std::shared_ptr<FloatingEnemy> pEnemy):
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

	//周期を超えたらリセットする
	if (m_frame > max_frame)
	{
		m_frame = 0;
	}

	//進行度を計算する
	float progress = static_cast<float>(m_frame) / static_cast<float>(max_frame);
	//cosを使用して縦に浮遊させる
	Vector3 vel;
	vel.m_y = std::cosf(progress * DX_TWO_PI_F) * move_amplitude;
	//敵の速度に適用する
	std::shared_ptr<FloatingEnemy> pEnemy = m_pEnemy.lock();
	pEnemy->SetVel(vel);
#ifdef _DEBUG
	DrawFormatString(0, 240, 0xff0000, L"velY : %f", vel.m_y);
#endif
}

void ActiveState::Exit()
{
}
