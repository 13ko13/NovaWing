#define NOMINMAX

#include "GameCamera.h"
#include "Game/GameObjects/Actors/Charactor/Player/Player.h"
#include "Constants/Game.h"

namespace
{
	// プレイヤーの移動に対してカメラの移動をどれぐらいにするか
	constexpr float camera_move_strength_y = 0.3f;
	constexpr float camera_move_strength_x = 0.3f;
	constexpr float camera_move_strength = 0.3f;

	// カメラのYオフセット
	constexpr float camera_offset_y = 200.0f;

	// プレイヤーからどれぐらい離したZ位置にカメラを置くか
	constexpr float camera_offset_z = 600.0f;

	//ズーム時の距離判定閾値
	constexpr float zoom_dist_thresould = 100.0f;

	// カメラのLerpに使うtの値
	constexpr float lerp_t = 0.06f;

	// カメラからターゲットまでの距離
	constexpr float camera_to_target = 38000.0f;
}

GameCamera::GameCamera(const std::shared_ptr<Player> pPlayer)
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

	//前フレームの位置に初期位置を保存しておく
	m_prevPos = m_pos;
}

GameCamera::~GameCamera()
{
}

void GameCamera::Draw()
{
}

void GameCamera::UpdatePosition()
{
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();
	// プレイヤーの位置
	Vector3 playerPos = pPlayer->GetPos();

	//もしズームスピードに値が入っていればズームを行う
	if (m_zoomSpeed > 0.0f)
	{
		//ターゲットオブジェクト
		//自分の位置からズーム対象の位置まで
		Vector3 zoomTargetPos = m_targetPos -
			Vector3(0.0f, 0.0f, m_zoomLimit);

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
}

void GameCamera::OnShake(float power, int frame)
{
	m_shakePower = power;
	m_shakeFrame = frame;

	//フラグを立てる
	m_isShake = true;
}

void GameCamera::OnZoomUp(
		float zoomSpeed,
		std::weak_ptr<GameObject> pTargetObject,
		float zoomLimit,
		float offsetY
)
{
	//ズームさせる速度を保持
	m_zoomSpeed = zoomSpeed;
	//ズームしたときの最低距離も保持
	m_zoomLimit = zoomLimit;

	//ターゲットオブジェクトの位置も保持
	//オフセット計算
	Vector3 offset = Vector3(0.0f, offsetY, 0.0f);
	m_targetPos = pTargetObject.lock()->GetPos() + offset;
}

Vector3 GameCamera::UpdateShake()
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

	Vector3 shakeVec = { randX, randY, randZ };
	// ベクトルを正規化して、m_shakePowerをかける
	shakeVec.Normalize();
	shakeVec *= m_shakePower;
	return shakeVec;
}

void GameCamera::UpdateTargetPos()
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