#include <cmath>

#include "MovingState.h"
#include "Manager/InputManager.h"
#include "Charactor/Player/Player.h"
#include "IdleMovementState.h"

namespace
{
	constexpr float move_speed_z = 8.0f;//移動速度

	constexpr float move_speed_x = 19.0f;//横方向の移動速度
	constexpr float move_speed_y = 19.0f;//縦方向の移動速度

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

	Vector3 vel;

	//上下入力
	vel.y = -stick.y * move_speed_y;
	//左右入力
	//TODO:プレイヤーを通じて、ローリング状態を取得。
	//ローリングされてたら、そのローリングの符号に応じた方向への移動速度を増やす
	vel.x = stick.x * move_speed_x;

	//進むときのスピードを設定する
	vel.z = move_speed_z;
	pPlayer->SetVel(vel);

	//lengthがしきい値未満ならIdleMovementStateに戻る
	if (length < idle_return_threshold)
	{
		ChangeState(std::make_shared<IdleMovementState>(m_pPlayer));
	}

#ifdef _DEBUG
	DrawFormatString(0, 100, 0xffffff, L"stickX:%f,stickY:%f", stick.x, stick.y);
#endif
}

void MovingState::Exit()
{}
