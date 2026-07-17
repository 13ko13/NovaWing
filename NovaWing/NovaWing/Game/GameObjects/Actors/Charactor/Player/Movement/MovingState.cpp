#include <cmath>

#include "MovingState.h"
#include "Manager/InputManager.h"
#include "Charactor/Player/Player.h"
#include "IdleMovementState.h"

namespace
{
	constexpr float move_speed_z = 8.0f;//移動速度

	constexpr float move_speed_x = 15.0f;//横方向の移動速度
	constexpr float move_speed_y = 15.0f;//縦方向の移動速度

	constexpr float stick_dead_zone = 0.1f;//スティックのデッドゾーン

	//移動制限範囲
	constexpr float move_limit_x = 500.0f;
	constexpr float move_limit_y = 300.0f;

	//アナログスティックの入力値を-1～1に正規化するための割る数
	constexpr float stick_input_max = 1000.0f;
	//この値未満ならIdleMovementStateに戻すしきい値
	constexpr float idle_return_threshold = 0.1f;
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
		stick.m_x * stick.m_x + stick.m_y * stick.m_y
	);
	if (length > 1.0f)
	{
		stick.m_x /= length;
		stick.m_y /= length;
	}

	//デッドゾーンより値が小さければ入力無しとする
	if (std::abs(stick.m_x) < stick_dead_zone)
	{
		stick.m_x = 0.0f;
	}
	if (std::abs(stick.m_y) < stick_dead_zone)
	{
		stick.m_y = 0.0f;
	}

	//weak_ptrからshared_ptrに変換
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();

	Vector3 vel;

	//上下入力
	vel.m_y = -stick.m_y * move_speed_y;
	//左右入力
	vel.m_x = stick.m_x * move_speed_x;

	//進むときのスピードを設定する
	vel.m_z = move_speed_z;
	pPlayer->SetVel(vel);

	//lengthがしきい値未満ならIdleMovementStateに戻る
	if (length < idle_return_threshold)
	{
		ChangeState(std::make_shared<IdleMovementState>(m_pPlayer));
	}

#ifdef _DEBUG
	DrawFormatString(0, 100, 0xffffff, L"stickX:%f,stickY:%f", stick.m_x, stick.m_y);
#endif
}

void MovingState::Exit()
{}
