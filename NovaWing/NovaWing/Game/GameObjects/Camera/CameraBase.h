#pragma once
#include <memory>

#include "Utility/Vector3.h"
#include "../GameObject.h"
#include "Utility/Vector2.h"

class Input;
class Player;
class CameraBase : public GameObject
{
public:
	CameraBase();
	~CameraBase();

	/// <summary>
	/// ターゲットの位置を受け取って、カメラの位置を更新する処理
	/// </summary>
	/// <param name="targetPos">注視点の位置</param>
	virtual void Update();

	//カメラの位置更新
	virtual void UpdatePosition() = 0;

	/// <summary>
	/// 自身の正面ベクトルを取得する
	/// </summary>
	/// <returns>正面ベクトル</returns>
	Vector3 const GetForward() const;

	//視野角を取得
	float GetFov() const;

	//視錐台の半分のサイズを返す
	Vector2 GetFrustumHalfSize(float distZ) const;

	//カメラの視野角とNear、Farを再設定する(シェーダー用)
	void SetUpCamera();

private:

protected:
	//ターゲットの位置
	Position3 m_targetPos;
	Position3 m_prevTargetPos;

	//前フレームの位置
	Position3 m_prevPos;
};