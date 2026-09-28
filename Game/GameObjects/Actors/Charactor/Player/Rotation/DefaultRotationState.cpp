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

	//ローリングボタン連続入力を二回押しと判定するまでの許容フレーム
	constexpr int double_press_frame = 30;

	//ローリング1回転(360)にかけるフレーム数
	constexpr float roll_frame = 20;
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

	//ローリングボタンが一回押されていたらフレーム更新
	if (m_pushRightRollFrame > 0)
	{
		//更新
		m_pushRightRollFrame++;
		if (m_pushRightRollFrame < double_press_frame)
		{
			//連続で二回入力されたら横に回転する
			if (input.IsTriggered(InputEvent::right_rolling))
			{
				//ローリング開始
				m_pushRightRollFrame = 0;
				m_isStartRolling = true;
				//累計回転量をリセットする
				m_rollSumAngle = 0.0f;
				//回転方向を記録
				m_rollDir = 1;
			}
		}
		else m_pushRightRollFrame = 0;
	}
	//ローリングボタンが一回押されていたらフレーム更新
	if (m_pushLeftRollFrame > 0)
	{
		//更新
		m_pushLeftRollFrame++;
		if (m_pushLeftRollFrame < double_press_frame)
		{
			//連続で二回入力されたら横に回転する
			if (input.IsTriggered(InputEvent::left_rolling))
			{
				//ローリング開始
				m_pushLeftRollFrame = 0;
				m_isStartRolling = true;
				//累計回転量をリセットする
				m_rollSumAngle = 0.0f;
				//回転方向を記録
				m_rollDir = -1;
			}
		}
		else m_pushLeftRollFrame = 0;
	}
	if (input.IsTriggered(InputEvent::right_rolling) &&
		m_pushRightRollFrame < 1 &&
		!m_isStartRolling)
	{
		m_pushRightRollFrame = 1;
	}
	if (input.IsTriggered(InputEvent::left_rolling) &&
		m_pushLeftRollFrame < 1 &&
		!m_isStartRolling)
	{
		m_pushLeftRollFrame = 1;
	}

	if (m_isStartRolling)
	{
		float rotSpeed = 0.0f;
		//1フレーム当たりの回転量
		if (m_rollDir > 0) rotSpeed = (DX_TWO_PI_F) / roll_frame;
		if (m_rollDir < 0) rotSpeed = (-DX_TWO_PI_F) / roll_frame;

		//一定角速度で回転させ続ける
		pPlayer->AddRotationZ(rotSpeed);
		//累計回転量を計算
		m_rollSumAngle += rotSpeed;

		//1周し終わったら終了
		if (m_rollSumAngle >= DX_TWO_PI_F)
		{
			m_isStartRolling = false;
			m_rollSumAngle = 0.0f;
			m_rollDir = 0;
		}
		if (m_rollSumAngle <= -DX_TWO_PI_F)
		{
			m_isStartRolling = false;
			m_rollSumAngle = 0.0f;
			m_rollDir = 0;
		}
	}
	//ローリングが1回入力されたら機体を傾ける
	//連続で二回入力されたら横に回転する
	else if (input.IsPressed(InputEvent::right_rolling))
	{
		//Z軸回転
		//回転を行う
		targetAngle = DX_PI_F / 2;
		pPlayer->LerpToAngleZ(targetAngle, rot_lerp_t);
	}
	else if (input.IsPressed(InputEvent::left_rolling))
	{
		//Z軸回転
		//回転を行う
		targetAngle = -DX_PI_F / 2;
		pPlayer->LerpToAngleZ(targetAngle, rot_lerp_t);
	}
	else
	{
		targetAngle = 0.0f;
		pPlayer->LerpToAngleZ(targetAngle, rot_lerp_t);
	}
}

void DefaultRotationState::Exit()
{
}
