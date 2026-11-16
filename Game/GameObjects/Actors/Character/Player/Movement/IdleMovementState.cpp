#include <cmath>
#include <algorithm>

#include "IdleMovementState.h"
#include "Manager/InputManager.h"
#include "Utility/Vector2.h"
#include "MovingState.h"
#include "Character/Player/Player.h"

namespace
{
	constexpr float move_speed = 18.0f;

	//左右上下の最高移動速度(MovingStateのmove_speed_xyと同じ値)
	constexpr float move_speed_xy = 18.0f;

	//アナログスティックの入力値を-1～1に正規化するための割る数
	constexpr float stick_input_max = 1000.0f;
	//この値を超えたらMovingStateに切り替えるしきい値
	constexpr float moving_trigger_threshold = 0.1f;

	//機体が完全に止まるのにかかる時間
	constexpr float stop_frame = 24.0f;
}

IdleMovementState::IdleMovementState(const std::weak_ptr<Player> pPlayer):
	IMovementState(pPlayer)
{
	//入った時の処理
	Enter();
}

IdleMovementState::~IdleMovementState()
{
}

void IdleMovementState::Enter()
{
	
}

void IdleMovementState::Update()
{
	InputManager& input = InputManager::GetInstance();

	//左スティックの値を取得して-1～1にする
	Vector2 stick = {
		static_cast<float>(input.GetBufY()) / stick_input_max,
		static_cast<float>(input.GetBufX()) / stick_input_max
	};

	//先に正規化しておく
	float length = std::sqrtf(
		stick.x * stick.x + stick.y * stick.y
	);
	if (length > 1.0f)
	{
		stick.x /= length;
		stick.y /= length;
	}

	//少しでも入力が入っていればステートを変更する
	if (length > moving_trigger_threshold)
	{
		ChangeState(std::make_shared<MovingState>(m_pPlayer));
	}

	auto pPlayer = m_pPlayer.lock();

	//Zは普通に移動させる
	Vector3 targetVel;
	targetVel.x = 0.0f;
	targetVel.y = 0.0f;
	targetVel.z = move_speed;
	Vector3 vel;
	vel.z = targetVel.z;

	//今の速度をとって、xとyだけ目標速度へと1フレームの上限を付けて近づける
	Vector3 currentVel = pPlayer->GetVel();
	//1フレームに進むことができる上限値
	float step = move_speed_xy / stop_frame;
	vel.x = currentVel.x + std::clamp(targetVel.x - currentVel.x, -step, step);
	vel.y = currentVel.y + std::clamp(targetVel.y - currentVel.y, -step, step);

	pPlayer->SetVel(vel);
}

void IdleMovementState::Exit()
{
}
