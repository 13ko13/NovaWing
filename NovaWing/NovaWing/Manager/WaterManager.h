#pragma once
#include <vector>
#include <memory>

#include "Utility/Vector3.h"

class CameraBase;
class WaterManager
{
public:
	WaterManager(const std::shared_ptr<CameraBase> pCamera);
	~WaterManager();

	//初期化処理
	void Init();
	//更新処理
	void Update();
	//描画
	void Draw();

private:
	//どの頂点を使用して四角形を作るかのインデックスデータを生成
	void CreateIndicesData();
	//頂点データを生成する
	void CreateVertexData();

	//定数バッファの作成
	void CreateConstantBuffer();

	//行列情報の更新
	void UpdateShaderMatrixData();

private:
	//フレームカウンター
	int m_frame = 0;

	//スカイボックスのテクスチャ
	int m_skyFrontH = -1;
	int m_skyBackH = -1;
	int m_skyRightH = -1;
	int m_skyLeftH = -1;
	int m_skyUpH = -1;
	int m_skyBottomH = -1;
	//コースティクステクスチャ
	int m_causticsH = -1;

	//↓なぜunsigned shortなのか
	//intは32ビット、unsigned shordは16ビット
	//海のグリッド程度ならばそこまで頂点数は多くならないので、
	//unsigned shortを使用する
	std::vector<unsigned short> m_indices;

	//グリッドのサイズの頂点数分の頂点データを作成する
	std::vector<VERTEX3DSHADER> m_vertexData;

	//頂点バッファのハンドル
	int m_vertexBufferH = -1;
	//インデックスバッファのハンドル
	int m_indexBufferH = -1;

	//海の頂点シェーダのハンドル
	int m_waterVSH = -1;
	//海のピクセルシェーダのハンドル
	int m_waterPSH = -1;

	//海シェーダに渡す情報
	struct WaterBuffer
	{
		float time = 0.0f;
		float padding[3];
	};
	//海の定数バッファハンドル
	int m_cbufferWater = -1;
	//定数バッファのポインタ
	WaterBuffer* m_pCBufferWaterData = nullptr;

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
		Vector3 cameraPos;//カメラ位置(12バイト)
		float screenWidth;//4バイト
		float screenHeight;//4バイト
		float padding[3];//詰め物(12バイト(16バイトアライメント))
	};//合計32バイト
	int m_cbufferCamera = -1;
	CameraBuffer* m_pCbufferCameraData = nullptr;

	//カメラの弱参照
	std::weak_ptr<CameraBase> m_pCamera;

	//板ポリが今までどのぐらい押し出されたか
	float m_meshZOffset = 0.0f;
};

