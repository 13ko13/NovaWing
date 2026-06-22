#include <cmath>

#include "DefaultRotationState.h"
#include "Manager/InputManager.h"
#include "Utility/SmartPointerHelper.h"
#include "Player.h"

namespace
{
	constexpr float rot_lerp_t = 0.05f;//Lerpに使うtの値
	constexpr float stick_dead_zone = 0.1f;//スティックのデッドゾーン
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
		static_cast<float>(input.GetBufX()) / 1000.0f,
		static_cast<float>(input.GetBufY()) / 1000.0f
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

	if (std::abs(stick.m_x) < stick_dead_zone)
	{
		stick.m_x = 0.0f;
	}
	if (std::abs(stick.m_y) < stick_dead_zone)
	{
		stick.m_y = 0.0f;
	}

	//weak_ptrからshared_ptrに変換
	std::shared_ptr<Player> pPlayer = WeakToShared(m_pPlayer);

	float targetAngle;

	//上下入力
	//X軸回転
	targetAngle = stick.m_y * (DX_PI_F / 4.0f);
	pPlayer->LerpToAngleX(targetAngle, rot_lerp_t);

	//左右入力
	//Y軸回転
	//モデルの前後が逆なので180度回した状態を基準として
	//回転を行う
	targetAngle = DX_PI_F + (stick.m_x * (DX_PI_F / 4.0f));
	pPlayer->LerpToAngleY(targetAngle, rot_lerp_t);
}

void DefaultRotationState::Exit()
{
}
