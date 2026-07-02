#pragma once
#include "../Actor.h"

/// <summary>
/// HPを持つキャラクター
/// </summary>
class Charactor : public Actor
{
public:
	Charactor(ResourceLoader::ModelID modelId);
	virtual ~Charactor();
	virtual void Update() override;
	virtual void Draw() = 0;
	virtual void TakeDamage(int damage) = 0;//ダメージを受ける

	int GetHealth() const { return m_health; }//HPを取得

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

	//機体情報
	int m_health = 100;//体力
	float m_attackPower = 1.0f;//攻撃力

protected:
	//シェーダに渡す定数バッファを作成
	void CreateShaderBuffers();
	//シェーダに渡す行列情報を更新
	void UpdateShaderMatrixData(const Vector3& cameraPos);
	//定数バッファをシェーダレジスタにセットする
	void BindShaderBuffers();
	//シェーダレジスタにセットしている定数バッファを解放する
	void ReleaseShaderBuffers();
};