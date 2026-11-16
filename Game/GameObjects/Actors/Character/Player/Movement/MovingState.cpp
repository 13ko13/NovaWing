#include <cmath>
#include <algorithm>

#include "MovingState.h"
#include "Manager/InputManager.h"
#include "Character/Player/Player.h"
#include "IdleMovementState.h"

namespace
{
	constexpr float move_speed_z = 18.0f;//移動速度

	constexpr float move_speed_xy = 18.0f;//左右上下の最高移動速度
	constexpr float tilt_move_speed_x = 25.0f;//機体が傾いているときの横移動速度

	constexpr float stick_dead_zone = 0.1f;//スティックのデッドゾーン

	//移動制限範囲
	constexpr float move_limit_x = 500.0f;
	constexpr float move_limit_y = 300.0f;

	//アナログスティックの入力値を-1～1に正規化するための割る数
	constexpr float stick_input_max = 1000.0f;
	//この値未満ならIdleMovementStateに戻すしきい値
	constexpr float idle_return_threshold = 0.1f;

	//プレイヤーが傾いていると判断する閾値
	constexpr float tilt_threshold = DX_PI_F / 4.0f;

	//最高速度に行くまでのフレーム
	constexpr float accel_frame = 20.0f;
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
	Vector2 stick = {
		static_cast<float>(input.GetBufX()) / stick_input_max,
		static_cast<float>(input.GetBufY()) / stick_input_max
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

	//デッドゾーンより値が小さければ入力無しとする
	if (std::abs(stick.x) < stick_dead_zone)
	{
		stick.x = 0.0f;
	}
	if (std::abs(stick.y) < stick_dead_zone)
	{
		stick.y = 0.0f;
	}

	//weak_ptrからshared_ptrに変換
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();

	Vector3 targetVel;

	//上下入力
	targetVel.y = -stick.y * move_speed_xy;
	//左右入力
	//ローリングされてたら、そのローリングの符号に応じた方向への移動速度を増やす
	if (pPlayer->GetRotationZ() > tilt_threshold &&
		stick.x > 0.0f)
	{
		targetVel.x = std::abs(stick.x) * tilt_move_speed_x;
	}
	else if (pPlayer->GetRotationZ() < -tilt_threshold &&
		stick.x < 0.0f)
	{
		targetVel.x = std::abs(stick.x) * -tilt_move_speed_x;
	}
	else
	{
		targetVel.x = stick.x * move_speed_xy;
	}

	//Zは普通に移動させる
	targetVel.z = move_speed_z;
	Vector3 vel;
	vel.z = targetVel.z;

	//今の速度をとって、xとyだけ目標速度へと1フレームの上限を付けて近づける
	Vector3 currentVel = pPlayer->GetVel();
	//1フレームに進むことができる上限値
	float step = move_speed_xy / accel_frame;
	vel.x = currentVel.x + std::clamp(targetVel.x - currentVel.x, -step, step);
	vel.y = currentVel.y + std::clamp(targetVel.y - currentVel.y, -step, step);

	pPlayer->SetVel(vel);
		
	//lengthがしきい値未満ならIdleMovementStateに戻る
	if (length < idle_return_threshold)
	{
		ChangeState(std::make_shared<IdleMovementState>(m_pPlayer));
	}
}

void MovingState::Exit()
{
}
