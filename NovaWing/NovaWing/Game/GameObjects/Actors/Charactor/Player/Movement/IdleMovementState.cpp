#include <cmath>

#include "IdleMovementState.h"
#include "Manager/InputManager.h"
#include "Utility/Vector2.h"
#include "MovingState.h"
#include "Charactor/Player/Player.h"

namespace
{
	constexpr float move_speed = 6.0f;
}

IdleMovementState::IdleMovementState(const std::weak_ptr<Player> pPlayer):
	IMovementState(pPlayer)
{
	Enter();
}

IdleMovementState::~IdleMovementState()
{
}

void IdleMovementState::Enter()
{
	//プレイヤーの速度をz以外0にする
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();
	pPlayer->SetVel(Vector3(0.0f, 0.0f, move_speed));
}

void IdleMovementState::Update()
{
	InputManager& input = InputManager::GetInstance();

	//左スティックの値を取得して-1～1にする
	Vector2 stick = { 
		static_cast<float>(input.GetBufY()) / 1000.0f,
		static_cast<float>(input.GetBufX()) / 1000.0f
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

	//少しでも入力が入っていればステートを変更する
	if (length > 0.1f)
	{
		ChangeState(std::make_shared<MovingState>(m_pPlayer));
	}
}

void IdleMovementState::Exit()
{
}
