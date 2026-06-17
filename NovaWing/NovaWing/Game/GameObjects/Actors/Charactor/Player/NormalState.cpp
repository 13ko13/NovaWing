#include "NormalState.h"
#include "../../../../../Manager/InputManager.h"
#include "Player.h"

namespace
{
	constexpr float move_speed = 4.0f;
}

NormalState::NormalState(std::shared_ptr<Player> pPlayer) :
	IPlayerState(pPlayer)
{
}

NormalState::~NormalState()
{
	//処理なし
}

void NormalState::Enter()
{
}

void NormalState::Update()
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
	if (stickY > 0.2f)
	{
		//X軸回転
		m_pPlayer->RotateX(0.03f * stickY);
		//画面下に動く
		vel.m_y = -move_speed;
	}
	//上入力
	else if (stickY < -0.2f)
	{
		//Y軸回転
		m_pPlayer->RotateX(0.03f * stickY);
		//画面上に動く
		vel.m_y = move_speed;
	}
	//入力なし
	else
	{
		//m_rotationXを0に向けてLerp
		m_pPlayer->LerpRotation(0.03f);
	}

	//右入力
	if (stickX > 0.2f)
	{
		//Y軸回転
		m_pPlayer->RotateY(0.03f * stickX);
		//画面右に動く
		vel.m_x = move_speed;
	}
	//左入力
	else if (stickX < -0.2f)
	{
		//Y軸回転
		m_pPlayer->RotateY(0.03f * stickX);
		//画面左に動く
		vel.m_x = -move_speed;
	}
	//入力なし
	else
	{
		//m_rotationYを0に向けてLerp
		m_pPlayer->LerpRotation(0.03f);
	}

	//進むときのスピードを設定する
	vel.m_z = move_speed;
	m_pPlayer->SetVel(vel);

#ifdef _DEBUG
	DrawFormatString(0, 45, 0xffffff, L"stickX:%f,stickY:%f", static_cast<float>(input.GetBufX()), stickY);
#endif
}

void NormalState::Exit()
{
}
