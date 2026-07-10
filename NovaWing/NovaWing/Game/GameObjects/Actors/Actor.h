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
class CameraBase;
class Actor :public GameObject
{
public:
	Actor(ResourceLoader::ModelID modelID,
	std::weak_ptr<CameraBase> pCamera);
	virtual ~Actor();

	//純粋仮想関数
	virtual void Update() = 0;//更新
	virtual void Draw() = 0;//描画

	//行列を適用する
	void ApplyMatrix(const Vector3& scale, const Vector3& pos,
		Quaternion& rotation, int modelHandle);

	//カメラをセットする
	void SetCamera(std::weak_ptr<CameraBase> pCamera){m_pCamera = pCamera;}
	
private:

protected:
	//シェーダに渡す情報
	struct MatrixBuffer
	{
		MATRIX world;//ワールド行列
		MATRIX view;//ビュー行列
		MATRIX proj;//プロジェクション行列
	};

	int m_cbufferMatrix = -1;
	MatrixBuffer* m_pCbufferMatrixData = nullptr;

	struct CameraBuffer
	{
		Vector3 cameraPos;//カメラの位置
		float padding;
	};
	int m_cbufferCamera = -1;
	CameraBuffer* m_pCbufferCameraData = nullptr;

	//カメラの弱参照
	std::weak_ptr<CameraBase> m_pCamera;

	//機体情報
	int m_modelHandle = -1;//モデルハンドル

protected:
	//シェーダに渡す定数バッファを作成
	void CreateShaderBuffers();
	//シェーダに渡す行列情報を更新
	void UpdateShaderMatrixData();
	//定数バッファをシェーダレジスタにセットする
	void BindShaderBuffers();
	//シェーダレジスタにセットしている定数バッファを解放する
	void ReleaseShaderBuffers();
	//カメラの位置を取得する
	Vector3 GetCameraPos() const;
};

