#define NOMINMAX

#include "CameraBase.h"
#include "Constants/Game.h"
#include "Game/GameObjects/Actors/Charactor/Player/Player.h"
#include "Main/Application.h"
#include "Utility/Vector3.h"

namespace
{
	constexpr float camera_near = 200.0f; // カメラのNear
	constexpr float camera_far = 5500.0f; // カメラのFar

	// カメラの視野角
	constexpr float fov = DX_PI_F / 2.0f;

	// カメラのLerpに使うtの値
	constexpr float lerp_t = 0.06f;

	// プレイヤーの移動に対してカメラの移動をどれぐらいにするか
	constexpr float camera_move_strength_y = 0.3f;
	constexpr float camera_move_strength_x = 0.3f;
	constexpr float camera_move_strength = 0.3f;
	// カメラのYオフセット
	constexpr float camera_offset_y = 200.0f;
	// プレイヤーからどれぐらい離したZ位置にカメラを置くか
	constexpr float camera_offset_z = 600.0f;

	// カメラからターゲットまでの距離
	constexpr float camera_to_target = 38000.0f;

	//ズーム時の最高限距離
	const Vector3 zoom_limit = Vector3(0.0f, 0.0f, 1000.0f);

	//ズーム時の距離判定閾値
	constexpr float zoom_dist_thresould = 100.0f;

} // namespace

CameraBase::CameraBase(const std::shared_ptr<Player> pPlayer)
{
	// 受け取ったshared_ptrプレイヤーをweakプレイヤーに入れる
	m_pPlayer = pPlayer;

	// ターゲットの位置を更新
	UpdateTargetPos();

	// プレイヤーの位置
	Vector3 playerPos = pPlayer->GetPos();

	// 初期位置を設定
	// プレイヤーが動くと、カメラもプレイヤーよりも小さい量で移動する
	m_pos.m_x = playerPos.m_x * camera_move_strength_x;
	// yはxよりも小さく動いて、海面より下にはならないようにする
	m_pos.m_y = playerPos.m_y * camera_move_strength_y * -1.0f + camera_offset_y;
	m_pos.m_y = std::max(m_pos.m_y, Game::sea_camera_margin);
	// zはプレイヤーよりも少し手前
	m_pos.m_z = playerPos.m_z - camera_offset_z;

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


	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();
	// プレイヤーの位置
	Vector3 playerPos = pPlayer->GetPos();



	//もしズームスピードに値が入っていればズームを行う
	if (m_zoomSpeed > 0.0f)
	{
		//ターゲットオブジェクト
		//自分の位置からズーム対象の位置まで
		Vector3 zoomTargetPos = m_targetPos - zoom_limit;
		float cameraToTargetDist = (zoomTargetPos - m_pos).Length();

		m_pos = Vector3::Lerp(m_pos, zoomTargetPos, m_zoomSpeed);

		if (cameraToTargetDist < zoom_dist_thresould)
		{
			m_zoomSpeed = 0.0f;
		}
	}
	//ズーム中はプレイヤー追従をやめる
	else
	{
		// ターゲットの位置を更新
		UpdateTargetPos();

		// ターゲットの位置も補間する
		m_targetPos = Vector3::Lerp(m_prevTargetPos, m_targetPos, lerp_t);
		//前フレームのカメラの位置を保存
		m_prevPos = m_pos;
		// プレイヤーが動くと、カメラもプレイヤーよりも小さい量で移動する
		m_pos.m_x = playerPos.m_x * camera_move_strength_x;
		// yはxよりも小さく動いて、海面より下にはならないようにする
		m_pos.m_y = playerPos.m_y * camera_move_strength_y + camera_offset_y;
		m_pos.m_y = std::max(m_pos.m_y, Game::sea_camera_margin);
		// zはプレイヤーよりも少し手前
		m_pos.m_z = playerPos.m_z - camera_offset_z;

		//前フレームのカメラの位置から今のフレームの位置まで補間する
		m_pos = Vector3::Lerp(m_prevPos, m_pos, lerp_t);

		// カメラの揺れを更新して、カメラの位置に加算する
		m_pos += UpdateShake();
	}

	// カメラの位置とターゲットの位置をセットする
	SetCameraPositionAndTarget_UpVecY(m_pos.ToDxLib(), m_targetPos.ToDxLib());
}

void CameraBase::Draw()
{
#ifdef _DEBUG
	// DrawFormatString((int)0.0f, (int)30.0f, 0xffffff, "angleX : %f,Y : %f", m_angleX,m_angleY);
	//  DrawFormatString((int)0.0f, (int)30.0f, 0xffffff, L"CameraPosX : %f,posY:%f,posZ:%f", m_pos.m_x, m_pos.m_y,m_pos.m_z);
#endif // _DEBUG
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

void CameraBase::OnShake(float power, int frame)
{
	m_shakePower = power;
	m_shakeFrame = frame;

	//フラグを立てる
	m_isShake = true;
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

void CameraBase::OnZoomUp(
	float zoomSpeed,
	std::weak_ptr<GameObject> pTargetObject,
	float offsetY)
{
	//ズームさせる速度を保持
	m_zoomSpeed = zoomSpeed;

	//ターゲットオブジェクトの位置も保持
	//オフセット計算
	Vector3 offset = Vector3(0.0f, offsetY, 0.0f);
	m_targetPos = pTargetObject.lock()->GetPos() + offset;
}

Vector3 CameraBase::UpdateShake()
{
	if (m_shakeFrame < 0)
	{
		//フラグを降ろす
		m_isShake = false;
		return Vector3(0.0f, 0.0f, 0.0f);
	}

	m_shakeFrame--;
	// ランダムな方向のベクトルを作る
	float randX = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f; //-1.0f～1.0fの乱数
	float randY = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f;
	float randZ = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f;

	Vector3 shakeVec = {randX, randY, randZ};
	// ベクトルを正規化して、m_shakePowerをかける
	shakeVec.Normalize();
	shakeVec *= m_shakePower;
	return shakeVec;
}

void CameraBase::UpdateTargetPos()
{
	// プレイヤーの位置を取得して、追従する
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();
	if (pPlayer != nullptr) // Nullチェック
	{
		// 宙返り中ではないときのみ
		// プレイヤーから見たかなり前のほうをターゲットとする
		if (!pPlayer->IsSomersault())
		{
			m_targetPos = pPlayer->GetPos() + (-pPlayer->GetForward() * camera_to_target);
		}
	}
}
