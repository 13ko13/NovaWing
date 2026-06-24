#include "CameraBase.h"
#include "Manager/InputManager.h"
#include "Utility/Matrix4x4.h"
#include "Game/GameObjects/Actors/Charactor/Player/Player.h"
#include "Utility/Vector3.h"
#include "Utility/Quaternion.h"

namespace
{
	constexpr float camera_near = 200.0f;//カメラのNear
	constexpr float camera_far = 3500.0f;//カメラのFar
	const Vector3 first_pos = { 0.0f,300.0f,700.0f };
	//注視点からカメラ位置に向かうベクトル
	const Vector3 target_to_camera = { 0.0f,200.0f,-1200.0f };
	//右スティックを動かしたときのカメラの回転角の増減量
	constexpr float camera_rotate_speed = 0.04f;
	//カメラが回転するまでのデッドゾーン
	constexpr float camera_rot_dead_zone = 0.5f;
	//カメラの固定Y位置
	constexpr float camera_fixed_y = 100.0f;

	//カメラの視野角
	constexpr float fov = DX_PI_F / 3.0f;

	//カメラの回転角の制限
	constexpr float rot_rimit_up = DX_PI_F / 18.0f;//真上向くときは10度以上回転しないようにする
	constexpr float rot_rimit_down = -DX_PI_F / 3.0f;//真下向くときは-60度以上回転しないようにする

	//カメラのLerpに使うtの値
	constexpr float lerp_t = 0.2f;
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

	//カメラの視野角を設定する(180/3)
	SetupCamera_Perspective(fov);
	//カメラの遠近感の範囲を設定する
	SetCameraNearFar(camera_near, camera_far);
}

CameraBase::~CameraBase()
{

}

void CameraBase::Update()
{
	InputManager& input = InputManager::GetInstance();

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
	//XとYは固定値にする
	m_pos.m_x = 0.0f;
	m_pos.m_y = camera_fixed_y;

	//カメラの揺れを更新して、カメラの位置に加算する
	m_pos += UpdateShake();

	//前のターゲットの位置を保存
	m_prevTargetPos = m_targetPos;

	//ターゲットの位置を更新
	UpdateTargetPos();

	//ターゲットの位置も補間する
	m_targetPos = Vector3::Lerp(m_prevTargetPos, m_targetPos, lerp_t);


	//カメラの位置とターゲットの位置をセットする
	SetCameraPositionAndTarget_UpVecY(m_pos.ToDxLib(), m_targetPos.ToDxLib());
}

void CameraBase::Draw()
{
#ifdef _DEBUG
	//DrawFormatString((int)0.0f, (int)30.0f, 0xffffff, "angleX : %f,Y : %f", m_angleX,m_angleY);
	DrawFormatString((int)0.0f, (int)30.0f, 0xffffff, L"posX : %f,posY:%f,posZ:%f", m_pos.m_x, m_pos.m_y,m_pos.m_z);
#endif // _DEBUG
}

Vector3 const CameraBase::GetForward() const
{
	//カメラの正面ベクトルは、注視点からカメラ位置に向かうベクトルの逆向きになる
	return (m_targetPos - m_pos).Normalized();
}

void CameraBase::OnShake(float power, int frame)
{
	m_shakePower = power;
	m_shakeFrame = frame;
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
	//注視点X、Yは固定
	m_targetPos.m_x = 0.0f;
	m_targetPos.m_y = 0.0f;

	//プレイヤーのZ位置を取得して、追従する
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();
	if(pPlayer != nullptr)//Nullチェック
	{
		m_targetPos.m_z = pPlayer->GetPos().m_z + 605.0f;
	}
}
