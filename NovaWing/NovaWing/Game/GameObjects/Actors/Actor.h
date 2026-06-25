#pragma once
#include <memory>

#include "../../../Utility/Vector3.h"
#include "../../../Utility/Quaternion.h"
#include "../GameObject.h"
#include "../../../Manager/ResourceLoader.h"

/// <summary>
/// モデルを持ち、ゲーム世界に見えるオブジェクト
/// </summary>
class ModelAnimator;
class Actor :public GameObject
{
public:
	Actor(ResourceLoader::ModelID modelId);
	virtual ~Actor();

	//純粋仮想関数
	virtual void Update() = 0;//更新
	virtual void Draw() = 0;//描画

	//セッター
	void SetVel(const Vector3& vel) { m_velocity = vel; };//速度を指定

	//ゲッター
	Vector3 GetVel() const { return m_velocity; }//速度を取得

	//行列を適用する
	void ApplyMatrix(const Vector3& scale, const Vector3& pos,
		Quaternion& rotation, int modelHandle);

private:

protected:
	//機体情報
	int m_modelHandle = -1;//モデルハンドル
};

