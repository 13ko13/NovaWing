#pragma once
#include <memory>

#include "../../../Utility/Vector3.h"
#include "../GameObject.h"
#include "Utility/Vector2.h"

class Input;
class Player;
class CameraBase : public GameObject
{
public:
	CameraBase(const std::shared_ptr<Player> pPlayer);
	~CameraBase();

	/// <summary>
	/// ターゲットの位置を受け取って、カメラの位置を更新する処理
	/// </summary>
	/// <param name="targetPos">注視点の位置</param>
	virtual void Update();

	/// <summary>
	/// 描画処理
	/// </summary>
	virtual void Draw();

	/// <summary>
	/// 自身の正面ベクトルを取得する
	/// </summary>
	/// <returns>正面ベクトル</returns>
	Vector3 const GetForward() const;

	//視野角を取得
	float GetFov() const;

	//視錐台の半分のサイズを返す
	Vector2 GetFrustumHalfSize(float distZ) const;

	/// <summary>
	/// カメラの揺れの開始
	/// </summary>
	/// <param name="power">揺れるときの力</param>
	/// <param name="frame">揺れの持続フレーム数</param>
	void OnShake(float power, int frame);

	//カメラの視野角とNear、Farを再設定する(シェーダー用)
	void SetUpCamera();

private:
	//揺れ
	float m_shakePower = 0.0f;//揺れるときの力
	int m_shakeFrame = 0;//揺れの持続フレーム数

	//ターゲットの位置
	Position3 m_targetPos;
	Position3 m_prevTargetPos;

	//プレイヤーは借りてくるだけなのでweak_ptrで持っておく
	std::weak_ptr<Player> m_pPlayer;

private:
	/// <summary>
	/// 揺れの更新
	/// </summary>
	/// <returns>揺れの速度ベクトル</returns>
	Vector3 UpdateShake();

	/// <summary>
	/// ターゲットの位置を更新する
	/// </summary>
	void UpdateTargetPos();
};