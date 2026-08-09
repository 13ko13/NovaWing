#include <DxLib.h>

#include "WaterManager.h"
#include "Utility/Size.h"
#include "Game/GameObjects/Camera/CameraBase.h"
#include "Manager/LightingManager.h"
#include "Utility/Matrix4x4.h"
#include "Manager/ResourceLoader.h"
#include "Constants/Game.h"
#include "Manager/WaterRevealManager.h"
#include "Constants/ShaderRegister.h"

namespace
{
	//グリッドの縦の分割数
	constexpr int vertical_grid_num = 40;
	//グリッドの横の分割数
	constexpr int horizontal_grid_num = 40;

	//グリッド全体の広さ
	constexpr Size grid_size = Size(10000,30000 );

	//波アニメーションの時間経過速度(小さいほどゆっくり動く)
	constexpr float time_speed = 0.01f;
	//プレイヤーが海の終端からどのくらい前に着いたら板ポリをワープさせるか
	constexpr float warp_threshould = 7000.0f;
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
	//画面解像度をシェーダに渡す情報にセットする
	m_pCbufferCameraData->screenWidth = static_cast<float>(Game::screen_width);
	m_pCbufferCameraData->screenHeight = static_cast<float>(Game::screen_height);

	//画像ハンドル取得
	ResourceLoader& loader = ResourceLoader::GetInstance();
	m_skyFrontH = loader.GetGraphic(ResourceLoader::GraphicID::SkyBoxFront);
	m_skyBackH = loader.GetGraphic(ResourceLoader::GraphicID::SkyBoxBack);
	m_skyRightH = loader.GetGraphic(ResourceLoader::GraphicID::SkyBoxRight);
	m_skyLeftH = loader.GetGraphic(ResourceLoader::GraphicID::SkyBoxLeft);
	m_skyUpH = loader.GetGraphic(ResourceLoader::GraphicID::SkyBoxUp);
	m_skyBottomH = loader.GetGraphic(ResourceLoader::GraphicID::SkyBoxBottom);
	m_causticsH = loader.GetGraphic(ResourceLoader::GraphicID::Caustics);
}
void WaterManager::Update()
{
	//フレームを更新
	m_frame++;

	//時間をシェーダ側に渡す
	//timeの変化が速すぎると波が高速で動いてしまうので
	//ゆっくり動かすために0.01をかける
	m_pCBufferWaterData->time = m_frame * time_speed;
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
	SetShaderConstantBuffer(m_cbufferWater, DX_SHADERTYPE_VERTEX, ShaderRegister::water_cbuffer_water_vs);
	SetShaderConstantBuffer(m_cbufferWater, DX_SHADERTYPE_PIXEL, ShaderRegister::water_cbuffer_water_ps);
	SetShaderConstantBuffer(m_cbufferMatrix, DX_SHADERTYPE_VERTEX, ShaderRegister::water_cbuffer_matrix);
	SetShaderConstantBuffer(m_cbufferCamera, DX_SHADERTYPE_PIXEL, ShaderRegister::water_cbuffer_camera);
	SetShaderConstantBuffer(
		LightingManager::GetInstance().GetLightCBuffer(),
		DX_SHADERTYPE_PIXEL, ShaderRegister::water_cbuffer_light
	);

	//スカイボックスのテクスチャをシェーダにセットする
	SetUseTextureToShader(ShaderRegister::water_tex_sky_front, m_skyFrontH);
	SetUseTextureToShader(ShaderRegister::water_tex_sky_back, m_skyBackH);
	SetUseTextureToShader(ShaderRegister::water_tex_sky_right, m_skyRightH);
	SetUseTextureToShader(ShaderRegister::water_tex_sky_left, m_skyLeftH);
	SetUseTextureToShader(ShaderRegister::water_tex_sky_up, m_skyUpH);
	SetUseTextureToShader(ShaderRegister::water_tex_sky_bottom, m_skyBottomH);
	SetUseTextureToShader(ShaderRegister::water_tex_caustics, m_causticsH);

	//透過水表現用のキャプチャテクスチャをシェーダにセットする
	SetUseTextureToShader(ShaderRegister::water_tex_scene_capture, WaterRevealManager::GetInstance().GetCaptureTextureHandle());

	//頂点バッファとインデックスバッファを使用して3Dポリゴンを描画する
	DrawPolygonIndexed3DToShader_UseVertexBuffer(
		m_vertexBufferH,
		m_indexBufferH);

	SetUseVertexShader(-1);
	SetUsePixelShader(-1);
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_VERTEX, ShaderRegister::water_cbuffer_water_vs);
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, ShaderRegister::water_cbuffer_water_ps);
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_VERTEX, ShaderRegister::water_cbuffer_matrix);
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, ShaderRegister::water_cbuffer_camera);
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, ShaderRegister::water_cbuffer_light);
	//テクスチャを解除
	for (int i = 0; i < ShaderRegister::water_tex_slot_count; i++)
	{
		SetUseTextureToShader(i, -1);
	}

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
			vertexData.pos.y = 0.0f;//高さは波の高さなのでシェーダ側で動かす
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
	//カメラの位置を取得
	Vector3 cameraPos = m_pCamera.lock()->GetPos();
	//メッシュの終端位置を計算
	float meshEndPos = m_meshZOffset + grid_size.m_height;
	//カメラのZがその終端から閾値内に近づいているかを判定
	while(cameraPos.m_z > meshEndPos - warp_threshould)
	{
		m_meshZOffset += grid_size.m_height - warp_threshould;
		meshEndPos = m_meshZOffset + grid_size.m_height;//終端も更新しないと無限ループが起きる
	}

	//平行移動行列を作成
	Matrix4x4 trans = Matrix4x4::Translate(Vector3(0.0f, 0.0f, m_meshZOffset));
	//メッシュの位置が変わったことを頂点シェーダに知らせる
	m_pCbufferMatrixData->world = trans.ToDxLib();
	m_pCbufferMatrixData->view = GetCameraViewMatrix();
	m_pCbufferMatrixData->proj = GetCameraProjectionMatrix();

	UpdateShaderConstantBuffer(m_cbufferMatrix);
}
