#include <cmath>

#include "GaugeActionStateBase.h"
#include "Manager/InputManager.h"
#include "Charactor/Player/Player.h"
#include "IdleMovementState.h"
#include "Utility/Vector2.h"

namespace
{
	constexpr float move_speed = 8.0f;//移動速度
	constexpr float stick_dead_zone = 0.1f;//スティックのデッドゾーン

	//ゲージ消費量
	constexpr float gauge_consumption = -0.5f;

	//アナログスティックの入力値を-1～1に正規化するための割る数
	constexpr float stick_input_max = 1000.0f;
}

GaugeActionStateBase::GaugeActionStateBase(
	const std::weak_ptr<Player> pPlayer):
	IMovementState(pPlayer)
{

}

GaugeActionStateBase::~GaugeActionStateBase()
{

}

void GaugeActionStateBase::Update()
{
	//ボタンの名前を取得(boost or brake)
	const char* buttonName = GetButtonName();
	//InputManager
	InputManager& input = InputManager::GetInstance();
	//Player
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();

	//ボタンを押されているならゲージを消費する
	if (input.IsPressed(buttonName))
	{
		//ゲージを消費
		pPlayer->ChangeGauge(gauge_consumption);
		//ゲージが0になったら
		if (pPlayer->GetGauge() <= 0)
		{
			//ステートを通常に戻す
			ChangeState(std::make_shared<IdleMovementState>(pPlayer));
		}
	}
	else
	{
		//押されていなければステートを通常に戻す
		ChangeState(std::make_shared<IdleMovementState>(pPlayer));
		return;//念のためreturn
	}

	//移動処理
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

	Vector3 vel;

	//上下入力
	vel.m_y = -stick.m_y * move_speed;
	//左右入力
	vel.m_x = stick.m_x * move_speed;

	vel.m_z = GetSpeed();
	pPlayer->SetVel(vel);
}
