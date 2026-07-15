#include "CameraBase.h"
#include "Manager/InputManager.h"
#include "Utility/Matrix4x4.h"
#include "Game/GameObjects/Actors/Charactor/Player/Player.h"
#include "Utility/Vector3.h"
#include "Utility/Quaternion.h"
#include "Main/Application.h"

namespace
{
	constexpr float camera_near = 200.0f;//カメラのNear
	constexpr float camera_far = 5500.0f;//カメラのFar
	const Vector3 first_pos = { 0.0f,600.0f,00.0f };
	//注視点からカメラ位置に向かうベクトル
	const Vector3 target_to_camera = { 0.0f,200.0f,-1200.0f };

	//カメラの視野角
	constexpr float fov = DX_PI_F / 2.0f;

	//カメラのLerpに使うtの値
	constexpr float lerp_t = 0.2f;
}

CameraBase::CameraBase(const std::shared_ptr<Player> pPlayer)
{
	//受け取ったshared_ptrプレイヤーをweakプレイヤーに入れる
	m_pPlayer = pPlayer;

	//ターゲットの位置を更新
	UpdateTargetPos();

	//初期座標
	m_pos = first_pos;

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
	//カメラの注視点
	Vector3 target = m_targetPos;
	//見ている方向から回転行列を作る
	Matrix4x4 rotMtxY = Matrix4x4::RotationY(m_angleY);
	Matrix4x4 rotMtxX = Matrix4x4::RotationX(m_angleX);
	//回転行列を掛け合わせる
	Matrix4x4 rotMtx = rotMtxX * rotMtxY;
	//注視点からカメラの位置に向かうベクトルに回転行列をかけることで
	//回転させたベクトルを求める
	Vector3 targetToCamera = rotMtx.Transform(target_to_camera);
	Vector3 endCameraPos = target + targetToCamera;

	//m_posからendCameraPosへのベクトル
	Vector3 tempCameraPos = endCameraPos - m_pos;
	tempCameraPos *= lerp_t;//カメラの移動速度を調整するために、ベクトルを0.1倍する

	//回転させたカメラへのベクトルとターゲットの位置を足して、
	//カメラの位置を求める
	m_pos += tempCameraPos;

	// m_pos.m_x = m_pPlayer.lock()->GetPos().m_x;
	// m_pos.m_y = m_pPlayer.lock()->GetPos().m_y + 100.0f;

	//カメラの揺れを更新して、カメラの位置に加算する
	m_pos += UpdateShake();

	//前のターゲットの位置を保存
	m_prevTargetPos = m_targetPos;

	//ターゲットの位置を更新
	UpdateTargetPos();

	//ターゲットの位置も補間する
	m_targetPos = Vector3::Lerp(m_prevTargetPos, m_targetPos, lerp_t);

	//プレイヤーが

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
		m_targetPos.m_z = pPlayer->GetPos().m_z + 605.0f;
		// m_targetPos.m_y = pPlayer->GetPos().m_y-300.0f;
		// m_targetPos.m_x = pPlayer->GetPos().m_x;
	}
}
