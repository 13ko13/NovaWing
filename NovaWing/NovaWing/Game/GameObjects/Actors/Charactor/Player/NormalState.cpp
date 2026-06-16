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

	float velX = 0.0f;
	float velY = 0.0f;

	Vector3 axis;
	//下入力
	if (stickY > 0.2f)
	{
		axis = Vector3(1.0f, 0.0f, 0.0f);
		m_pPlayer->Rotate(axis, -stickY * 0.03f);
		//画面下に動く
		velY = -move_speed;
	}
	//上入力
	else if (stickY < -0.2f)
	{
		axis = Vector3(1.0f, 0.0f, 0.0f);
		m_pPlayer->Rotate(axis, -stickY * 0.02f);
		//画面上に動く
		velY = move_speed;
	}
	//入力なし
	else
	{
		//Lerpをかけて回転を決める
		Quaternion rotation = Quaternion::Lerp(
			m_pPlayer->GetRotation(),
			m_pPlayer->GetInitRotation(),
			0.03f);

	    m_pPlayer->SetRotation(rotation);
	}

	//右入力
	if (stickX > 0.2f)
	{
		axis = Vector3(0.0f, 1.0f, 0.0f);
		m_pPlayer->Rotate(axis, stickX * 0.015f);
		//画面右に動く
		velX = move_speed;
	}
	//左入力
	else if (stickX < -0.2f)
	{
		axis = Vector3(0.0f, 1.0f, 0.0f);
		m_pPlayer->Rotate(axis, stickX * 0.015f);
		//画面左に動く
		velX = -move_speed;
	}

	//進むときのスピードを設定する
	Vector3 vel = Vector3(velX, velY, move_speed);
	m_pPlayer->SetVel(vel);

#ifdef _DEBUG
	DrawFormatString(0, 45, 0xffffff, L"stickX:%f,stickY:%f", static_cast<float>(input.GetBufX()), stickY);
#endif
}

void NormalState::Exit()
{
}
