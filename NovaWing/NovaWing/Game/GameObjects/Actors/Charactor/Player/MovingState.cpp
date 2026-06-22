#include <cmath>

#include "MovingState.h"
#include "Manager/InputManager.h"
#include "Utility/SmartPointerHelper.h"
#include "Player.h"
#include "IdleMovementState.h"

namespace
{
	constexpr float move_speed = 4.0f;

	//移動制限範囲
	constexpr float move_limit_x = 500.0f;
	constexpr float move_limit_y = 300.0f;
}

MovingState::MovingState(const std::weak_ptr<Player> pPlayer) :
	IMovementState(pPlayer)
{
}

MovingState::~MovingState()
{
}

void MovingState::Enter()
{
}

void MovingState::Update()
{
	InputManager& input = InputManager::GetInstance();

	//左スティックの値を取得して-1～1にする
	float stickY = static_cast<float>(input.GetBufY()) / 1000.0f;
	float stickX = static_cast<float>(input.GetBufX()) / 1000.0f;

	//先に正規化しておく
	float length = std::sqrtf(
		stickX * stickX + stickY * stickY
	);
	if (length > 1.0f)
	{
		stickX /= length;
		stickY /= length;
	}

	Vector3 vel;
	Vector3 axis;
	//下入力
	if (stickY > 0.01f)
	{
		//画面下に動く
		vel.m_y = -stickY * move_speed;
	}
	//上入力
	else if (stickY < -0.01f)
	{
		//画面上に動く
		vel.m_y = -stickY * move_speed;
	}

	//右入力
	if (stickX > 0.01f)
	{
		//画面右に動く
		vel.m_x = stickX * move_speed;
	}
	//左入力
	else if (stickX < -0.01f)
	{
		//画面左に動く
		vel.m_x = stickX * move_speed;
	}

	//進むときのスピードを設定する
	vel.m_z = move_speed;

	const std::shared_ptr<Player> pPlayer =
		WeakToShared(m_pPlayer);
	pPlayer->SetVel(vel);

	//lengthが0.1以下ならIdleMovementStateに戻る
	if (length < 0.1f)
	{
		ChangeState(std::make_shared<IdleMovementState>(m_pPlayer));
	}


	DrawFormatString(0, 100, 0xffffff, L"stickX:%f,stickY:%f", stickX, stickY);
}

void MovingState::Exit()
{}
