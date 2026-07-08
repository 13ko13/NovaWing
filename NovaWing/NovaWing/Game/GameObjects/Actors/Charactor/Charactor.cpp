#include "Charactor.h"

namespace
{
	constexpr int max_health = 100;//最大体力
}

Charactor::Charactor(ResourceLoader::ModelID modelID):
	Actor(modelID),
	m_health(max_health),
	m_attackPower(1.0f)
	
{
}

Charactor::~Charactor()
{
	//処理なし
}

void Charactor::Update()
{
	//位置の更新
	m_pos += m_velocity;
}

void Charactor::CreateShaderBuffers()
{
	//定数バッファを作成
	m_cbufferMatrix = CreateShaderConstantBuffer(sizeof(MatrixBuffer));
	m_pCbufferMatrixData = static_cast<MatrixBuffer*>(GetBufferShaderConstantBuffer(m_cbufferMatrix));
	m_cbufferCamera = CreateShaderConstantBuffer(sizeof(CameraBuffer));
	m_pCbufferCameraData = static_cast<CameraBuffer*>(GetBufferShaderConstantBuffer(m_cbufferCamera));
}

void Charactor::UpdateShaderMatrixData(const Vector3& cameraPos)
{
	//行列情報を入れる
	m_pCbufferMatrixData->world = MV1GetLocalWorldMatrix(m_modelHandle);
	m_pCbufferMatrixData->view = GetCameraViewMatrix();
	m_pCbufferMatrixData->proj = GetCameraProjectionMatrix();

	//カメラの位置をMatrix情報に渡す
	m_pCbufferCameraData->cameraPos = cameraPos;


	UpdateShaderConstantBuffer(m_cbufferMatrix);
	UpdateShaderConstantBuffer(m_cbufferCamera);
}

void Charactor::BindShaderBuffers()
{
	SetShaderConstantBuffer(m_cbufferMatrix, DX_SHADERTYPE_VERTEX, 2);
	SetShaderConstantBuffer(m_cbufferCamera, DX_SHADERTYPE_VERTEX, 3);
	SetShaderConstantBuffer(m_cbufferCamera, DX_SHADERTYPE_PIXEL, 3);
}

void Charactor::ReleaseShaderBuffers()
{
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_VERTEX, 2);
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_VERTEX, 3);
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, 3);
}
