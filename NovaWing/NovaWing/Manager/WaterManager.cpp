#include <DxLib.h>

#include "WaterManager.h"
#include "Utility/Size.h"
#include "Game/GameObjects/Camera/CameraBase.h"
#include "Manager/LightingManager.h"

namespace
{
	//グリッドの縦の分割数
	constexpr int vertical_grid_num = 20;
	//グリッドの横の分割数
	constexpr int horizontal_grid_num = 20;

	//グリッド全体の広さ
	constexpr Size grid_size = { 3000.0f,5000.0f };
}

WaterManager::WaterManager(const std::shared_ptr<CameraBase> pCamera) :
	m_pCamera(pCamera)
{
}

WaterManager::~WaterManager()
{
}

void WaterManager::Init()
{
	//頂点データを作成
	CreateVertexData();
	//どの頂点を使用して四角形を作るのかのインデックスデータを生成
	CreateIndicesData();

	//GPUに渡す頂点バッファを作成する
	m_vertexBufferH = CreateVertexBuffer(m_vertexData.size(), DX_VERTEX_TYPE_SHADER_3D);
	//GPUに渡すインデックスバッファを作成
	m_indexBufferH = CreateIndexBuffer(m_indices.size(), DX_INDEX_TYPE_16BIT);

	//頂点バッファにデータを書き込む
	SetVertexBufferData(0, m_vertexData.data(), m_vertexData.size(), m_vertexBufferH);
	//インデックスバッファにデータを書き込む
	SetIndexBufferData(0, m_indices.data(), m_indices.size(), m_indexBufferH);

	//シェーダのロード
	m_waterPSH = LoadPixelShader(L"WaterPS.pso");
	m_waterVSH = LoadVertexShader(L"WaterVS.vso");

	//定数バッファの作成
	CreateConstantBuffer();
}
void WaterManager::Update()
{
	//フレームを更新
	m_frame++;

	//時間をシェーダ側に渡す
	//timeの変化が速すぎると波が高速で動いてしまうので
	//ゆっくり動かすために0.01をかける
	m_pCBufferWaterData->time = m_frame * 0.01f;
	UpdateShaderConstantBuffer(m_cbufferWater);
}

void WaterManager::Draw()
{
	//シェーダに渡すカメラの情報を更新
	m_pCbufferCameraData->cameraPos = m_pCamera.lock()->GetPos();
	UpdateShaderConstantBuffer(m_cbufferCamera);

	//行列情報を更新する
	UpdateShaderMatrixData();

	//頂点シェーダをセットする
	SetUseVertexShader(m_waterVSH);
	//ピクセルシェーダをセットする
	SetUsePixelShader(m_waterPSH);
	//定数バッファを頂点シェーダにセットする
	SetShaderConstantBuffer(m_cbufferWater, DX_SHADERTYPE_VERTEX, 4);
	SetShaderConstantBuffer(m_cbufferMatrix, DX_SHADERTYPE_VERTEX, 2);
	SetShaderConstantBuffer(m_cbufferCamera, DX_SHADERTYPE_PIXEL, 5);
	SetShaderConstantBuffer(
		LightingManager::GetInstance().GetLightCBuffer(),
		DX_SHADERTYPE_PIXEL, 3
	);

	//頂点バッファとインデックスバッファを使用して3Dポリゴンを描画する
	DrawPolygonIndexed3DToShader_UseVertexBuffer(
		m_vertexBufferH,
		m_indexBufferH);

	SetUseVertexShader(-1);
	SetUsePixelShader(-1);
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_VERTEX, 4);
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_VERTEX, 2);
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, 5);
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, 3);

#ifdef _DEBUG
	DrawFormatString(0, 500, 0xff0000, L"frame : %f", m_pCBufferWaterData->time);
#endif
}

void WaterManager::CreateIndicesData()
{
	//インデックスデータを生成
	//四角形の数分ループしたいので、
	//頂点数-1をする
	for (int i = 0; i < vertical_grid_num - 1; i++)
	{
		for (int j = 0; j < horizontal_grid_num - 1; j++)
		{
			unsigned short leftTopIndex = i * horizontal_grid_num + j;//左上
			unsigned short rightTopIndex = i * horizontal_grid_num + j + 1;//右上
			unsigned short leftBotIndex = (i + 1) * horizontal_grid_num + j;//左下
			unsigned short rightBotIndex = (i + 1) * horizontal_grid_num + j + 1;//右下

			//この4つの頂点を使って四角形を作成
			m_indices.push_back(leftTopIndex);
			m_indices.push_back(leftBotIndex);
			m_indices.push_back(rightTopIndex);

			m_indices.push_back(rightTopIndex);
			m_indices.push_back(leftBotIndex);
			m_indices.push_back(rightBotIndex);
		}
	}
}

void WaterManager::CreateVertexData()
{
	//グリッド数分の頂点データを生成
	for (int i = 0; i < vertical_grid_num; i++)
	{
		for (int j = 0; j < horizontal_grid_num; j++)
		{
			VERTEX3DSHADER vertexData;
			//頂点の座標
			vertexData.pos.x =
				j * (grid_size.m_width / horizontal_grid_num) - (grid_size.m_width / 2);//-1500～1500
			vertexData.pos.y = -1000.0f;//高さは波の高さなのでシェーダ側で動かす
			vertexData.pos.z = i * (grid_size.m_height / vertical_grid_num);

			//頂点の法線
			//真上を向かせる
			vertexData.norm.x = 0.0f;
			vertexData.norm.y = 1.0f;
			vertexData.norm.z = 0.0f;

			//UV
			//0～1
			//グリッド全体に対してどの位置にあるかを表す
			vertexData.u = static_cast<float>(j) /
				static_cast<float>(horizontal_grid_num);//U
			vertexData.v = static_cast<float>(i) /
				static_cast<float>(vertical_grid_num);//V

			//ディフューズカラーを設定
			vertexData.dif = GetColorU8(0, 255, 255, 255);
			//スペキュラ
			vertexData.spc = GetColorU8(0, 0, 0, 0);
			//補助座標
			vertexData.spos = { 0.0f,0.0f,0.0f,0.0f };
			//接線
			vertexData.tan = { 1.0f,0.0f,0.0f };
			//従法線
			vertexData.binorm = { 0.0f,0.0f,1.0f };
			//補助テクスチャ座標
			vertexData.su = 0.0f;
			vertexData.sv = 0.0f;

			//頂点データを収めている配列に入れる
			m_vertexData.push_back(vertexData);
		}
	}
}

void WaterManager::CreateConstantBuffer()
{
	//シェーダに渡す定数バッファのサイズを確保
	m_cbufferWater = CreateShaderConstantBuffer(sizeof(WaterBuffer));
	//定数バッファのポインタを作成
	m_pCBufferWaterData = static_cast<WaterBuffer*>(GetBufferShaderConstantBuffer(m_cbufferWater));

	//シェーダに渡す定数バッファのサイズを確保
	m_cbufferMatrix = CreateShaderConstantBuffer(sizeof(MatrixBuffer));
	//定数バッファのポインタを作成
	m_pCbufferMatrixData = static_cast<MatrixBuffer*>(GetBufferShaderConstantBuffer(m_cbufferMatrix));

	//シェーダに渡す定数バッファのサイズを確保
	m_cbufferCamera = CreateShaderConstantBuffer(sizeof(CameraBuffer));
	//定数バッファのポインタを作成
	m_pCbufferCameraData = static_cast<CameraBuffer*>(GetBufferShaderConstantBuffer(m_cbufferCamera));
}

void WaterManager::UpdateShaderMatrixData()
{
	//行列情報を入れる
	m_pCbufferMatrixData->world = MGetIdent();
	m_pCbufferMatrixData->view = GetCameraViewMatrix();
	m_pCbufferMatrixData->proj = GetCameraProjectionMatrix();

	UpdateShaderConstantBuffer(m_cbufferMatrix);
}
