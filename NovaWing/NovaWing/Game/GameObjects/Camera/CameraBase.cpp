#include <cmath>

#include "CameraBase.h"
#include "Constants/Game.h"
#include "Main/Application.h"
#include "Utility/Vector3.h"

namespace
{
	constexpr float camera_near = 200.0f; // カメラのNear
	constexpr float camera_far = 5500.0f; // カメラのFar

	// カメラの視野角
	constexpr float fov = DX_PI_F / 2.0f;

} // namespace

CameraBase::CameraBase()
{
	// カメラの設定
	// プレイヤーの位置を取得して、そこをカメラのターゲットにする
	SetCameraPositionAndTarget_UpVecY(m_pos.ToDxLib(), m_targetPos.ToDxLib());

	// カメラの視野角とNear,Farを設定
	SetUpCamera();
}

CameraBase::~CameraBase()
{
}

void CameraBase::Update()
{
	// 前のターゲットの位置を保存
	m_prevTargetPos = m_targetPos;
	//前フレームのカメラの位置を保存
	m_prevPos = m_pos;

	//派生クラス側で書かせたカメラの位置更新を呼ぶ
	UpdatePosition();

	// カメラの位置とターゲットの位置をセットする
	SetCameraPositionAndTarget_UpVecY(m_pos.ToDxLib(), m_targetPos.ToDxLib());
}

Vector3 const CameraBase::GetForward() const
{
	// カメラの正面ベクトルは、注視点からカメラ位置に向かうベクトルの逆向きになる
	return (m_targetPos - m_pos).Normalized();
}

float CameraBase::GetFov() const
{
	return fov;
}

Vector2 CameraBase::GetFrustumHalfSize(float distZ) const
{
	// 画面の縦がどれぐらいの高さかを求める
	// fovと、プレイヤーがカメラからどれぐらい離れているかで
	// 画面の縦方向の高さは変わる
	// 中心から上端(下端)までの長さがわかれば
	// 画面の上端、下端に到達する位置が具体的に求まる
	// カメラからプレイヤーまでの長さを直角三角形の底辺
	// カメラから視界の上端までの長さを斜辺とする
	float screenHToWorld = std::tanf(fov / 2) * distZ;
	// そこにアスペクト比をかけて画面の横方向の半分の幅を求める
	const Size& wsize = Application::GetInstance().GetWindowSize();
	float screenWToWorld = screenHToWorld * (static_cast<float>(wsize.m_width) /
											 static_cast<float>(wsize.m_height));

	return Vector2(screenWToWorld, screenHToWorld);
}

void CameraBase::SetUpCamera()
{
	// カメラの視野角を設定する(180/3)
	SetupCamera_Perspective(fov);
	// カメラの遠近感の範囲を設定する
	SetCameraNearFar(camera_near, camera_far);
	// 位置も設定
	SetCameraPositionAndTarget_UpVecY(m_pos.ToDxLib(), m_targetPos.ToDxLib());
}