#pragma once
#include "CameraBase.h"

class GameCamera : public CameraBase
{
public:
	GameCamera(const std::shared_ptr<Player> pPlayer);
	~GameCamera();

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw() override;


	/// <summary>
	/// カメラの揺れの開始
	/// </summary>
	/// <param name="power">揺れるときの力</param>
	/// <param name="frame">揺れの持続フレーム数</param>
	void OnShake(float power, int frame);

	//カメラをズームさせる
	void OnZoomUp(
		float zoomSpeed,
		std::weak_ptr<GameObject> pTargetObject,
		float zoomLimit,
		float offsetY = 0.0f
	);

	//現在揺れているかを返す
	bool IsShake() const { return m_isShake; }

private:
	/// <summary>
	/// 揺れの更新
	/// </summary>
	/// <returns>揺れの速度ベクトル</returns>
	Vector3 UpdateShake();

	//カメラの位置更新
	void UpdatePosition() override;

	/// <summary>
	/// ターゲットの位置を更新する
	/// </summary>
	void UpdateTargetPos();

private:
	//揺れ
	float m_shakePower = 0.0f;//揺れるときの力
	int m_shakeFrame = 0;//揺れの持続フレーム数
	bool m_isShake = false;//現在揺れているか

	//ズーム速度
	float m_zoomSpeed = 0.0f;

	//ズームするときに保たせる最低距離
	float m_zoomLimit = 0.0f;

	//プレイヤーは借りてくるだけなのでweak_ptrで持っておく
	std::weak_ptr<Player> m_pPlayer;
};

