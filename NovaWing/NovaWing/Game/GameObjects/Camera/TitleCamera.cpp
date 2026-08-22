#include "TitleCamera.h"
#include "Game/GameObjects/Actors/Charactor/Player/TitlePlayer.h"

namespace
{
	//カメラの位置
	const Vector3 camera_pos = Vector3(-700.0f, 0.0f, 0.0f);
}

TitleCamera::TitleCamera(std::shared_ptr<TitlePlayer> pPlayer):
	m_pPlayer(pPlayer)
{
	//初期位置は一旦0にしておく
	m_pos = camera_pos;
	m_prevPos = m_pos;
}

TitleCamera::~TitleCamera()
{
}

void TitleCamera::UpdatePosition()
{
	//追従しているとき
	if (m_isFollowing)
	{
		//プレイヤーをターゲットとする
		m_targetPos = m_pPlayer->GetPos();
	}
}

void TitleCamera::Draw()
{
}

void TitleCamera::StopFollowing()
{
	//プレイヤー追従をやめる
	m_isFollowing = false;
}
