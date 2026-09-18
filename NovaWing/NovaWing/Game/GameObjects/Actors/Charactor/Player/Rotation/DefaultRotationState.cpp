#include <cmath>

#include "DefaultRotationState.h"
#include "Manager/InputManager.h"
#include "Charactor/Player/Player.h"

namespace
{
	constexpr float rot_lerp_t = 0.1f;//Lerpに使うtの値
	constexpr float stick_dead_zone = 0.1f;//スティックのデッドゾーン

	//アナログスティックの入力値を-1～1に正規化するための割る数
	constexpr float stick_input_max = 1000.0f;
	//スティック入力から求める機体の最大傾き角度
	constexpr float max_tilt_angle = DX_PI_F / 8.0f;
}

DefaultRotationState::DefaultRotationState(const std::weak_ptr<Player> pPlayer) :
	IRotationState(pPlayer)
{
}

DefaultRotationState::~DefaultRotationState()
{
}

void DefaultRotationState::Enter()
{
}

void DefaultRotationState::Update()
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

	float targetAngle;

	//上下入力
	//X軸回転
	targetAngle = stick.y * max_tilt_angle;
	pPlayer->LerpToAngleX(targetAngle, rot_lerp_t);

	//左右入力
	//Y軸回転
	//モデルの前後が逆なので180度回した状態を基準として
	//回転を行う
	targetAngle = DX_PI_F + (stick.x * max_tilt_angle);
	pPlayer->LerpToAngleY(targetAngle, rot_lerp_t);
}

void DefaultRotationState::Exit()
{
}
