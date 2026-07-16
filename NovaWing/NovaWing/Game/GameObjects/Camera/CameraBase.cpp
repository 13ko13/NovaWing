#define NOMINMAX

#include "CameraBase.h"
#include "Game/GameObjects/Actors/Charactor/Player/Player.h"
#include "Utility/Vector3.h"
#include "Main/Application.h"
#include "Constants/Game.h"

namespace
{
	constexpr float camera_near = 200.0f;//カメラのNear
	constexpr float camera_far = 5500.0f;//カメラのFar

	//カメラの視野角
	constexpr float fov = DX_PI_F / 2.0f;

	//カメラのLerpに使うtの値
	constexpr float lerp_t = 0.04f;

	//プレイヤーの移動に対してカメラの移動をどれぐらいにするか
	constexpr float camera_move_strength_y = 0.01f;
	constexpr float camera_move_strength_x = 0.3f;
	//カメラのYオフセット
	constexpr float camera_offset_y = 200.0f;
	//プレイヤーからどれぐらい離したZ位置にカメラを置くか
	constexpr float camera_offset_z = 600.0f;

	//カメラからターゲットまでの距離
	constexpr float camera_to_target = 15000.0f;
}

CameraBase::CameraBase(const std::shared_ptr<Player> pPlayer)
{
	//受け取ったshared_ptrプレイヤーをweakプレイヤーに入れる
	m_pPlayer = pPlayer;

	//ターゲットの位置を更新
	UpdateTargetPos();

	//カメラの設定
	//プレイヤーの位置を取得して、そこをカメラのターゲットにする
	SetCameraPositionAndTarget_UpVecY(m_pos.ToDxLib(), m_targetPos.ToDxLib());

	//カメラの視野角とNear,Farを設定
	SetUpCamera();
}

CameraBase::~CameraBase()
{

}

void CameraBase::Update()
{
	//カメラの揺れを更新して、カメラの位置に加算する
	m_pos += UpdateShake();

	//前のターゲットの位置を保存
	m_prevTargetPos = m_targetPos;

	//ターゲットの位置を更新
	UpdateTargetPos();

	//ターゲットの位置も補間する
	m_targetPos = Vector3::Lerp(m_prevTargetPos, m_targetPos, lerp_t);

	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();
	//プレイヤーの位置
	Vector3 playerPos = pPlayer->GetPos();

	//プレイヤーが動くと、カメラもプレイヤーよりも小さい量で移動する
	m_pos.m_x = playerPos.m_x *camera_move_strength_x;
	//yはxよりも小さく動いて、海面より下にはならないようにする
	playerPos.m_y * camera_move_strength_y * -1.0f + camera_offset_y;
	m_pos.m_y = std::max(m_pos.m_y,Game::sea_camera_margin);
	//zはプレイヤーよりも少し手前
	m_pos.m_z = playerPos.m_z -camera_offset_z;

	//カメラの位置とターゲットの位置をセットする
	SetCameraPositionAndTarget_UpVecY(m_pos.ToDxLib(), m_targetPos.ToDxLib());
}

void CameraBase::Draw()
{
#ifdef _DEBUG
	//DrawFormatString((int)0.0f, (int)30.0f, 0xffffff, "angleX : %f,Y : %f", m_angleX,m_angleY);
	// DrawFormatString((int)0.0f, (int)30.0f, 0xffffff, L"CameraPosX : %f,posY:%f,posZ:%f", m_pos.m_x, m_pos.m_y,m_pos.m_z);
#endif // _DEBUG
}

Vector3 const CameraBase::GetForward() const
{
	//カメラの正面ベクトルは、注視点からカメラ位置に向かうベクトルの逆向きになる
	return (m_targetPos - m_pos).Normalized();
}

float CameraBase::GetFov() const
{
 	return fov;
}

Vector2 CameraBase::GetFrustumHalfSize(float distZ) const
{
    //画面の縦がどれぐらいの高さかを求める
	//fovと、プレイヤーがカメラからどれぐらい離れているかで
	//画面の縦方向の高さは変わる
	//中心から上端(下端)までの長さがわかれば
	//画面の上端、下端に到達する位置が具体的に求まる
	//カメラからプレイヤーまでの長さを直角三角形の底辺
	//カメラから視界の上端までの長さを斜辺とする
	float screenHToWorld = std::tanf(fov / 2) * distZ;
	//そこにアスペクト比をかけて画面の横方向の半分の幅を求める
	const Size& wsize = Application::GetInstance().GetWindowSize();
	float screenWToWorld = screenHToWorld * (
		static_cast<float>(wsize.m_width) / 
		static_cast<float>(wsize.m_height));

	return Vector2(screenWToWorld,screenHToWorld);
}

void CameraBase::OnShake(float power, int frame)
{
	m_shakePower = power;
	m_shakeFrame = frame;
}

void CameraBase::SetUpCamera()
{
	//カメラの視野角を設定する(180/3)
	SetupCamera_Perspective(fov);
	//カメラの遠近感の範囲を設定する
	SetCameraNearFar(camera_near, camera_far);
	//位置も設定
	SetCameraPositionAndTarget_UpVecY(m_pos.ToDxLib(), m_targetPos.ToDxLib());
}

Vector3 CameraBase::UpdateShake()
{
	if (m_shakeFrame < 0)
	{
		return Vector3(0.0f, 0.0f, 0.0f);
	}

	m_shakeFrame--;
	//ランダムな方向のベクトルを作る
	float randX = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f;//-1.0f～1.0fの乱数
	float randY = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f;
	float randZ = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f;

	Vector3 shakeVec = { randX,randY,randZ };
	//ベクトルを正規化して、m_shakePowerをかける
	shakeVec.Normalize();
	shakeVec *= m_shakePower;
	return shakeVec;
}

void CameraBase::UpdateTargetPos()
{
	//プレイヤーの位置を取得して、追従する
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();
	if(pPlayer != nullptr)//Nullチェック
	{
		//注視点を求める
		m_targetPos = pPlayer->GetPos() + (-pPlayer->GetForward() * camera_to_target);
	}
}
