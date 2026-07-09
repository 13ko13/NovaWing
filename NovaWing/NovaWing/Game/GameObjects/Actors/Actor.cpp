#include <DxLib.h>

#include "Actor.h"
#include "Utility/Matrix4x4.h"

Actor::Actor(ResourceLoader::ModelID modelID) 
{
	//受け取ったモデルIDをもとにモデルを複製してハンドルを取得する
	m_modelHandle = MV1DuplicateModel(
		ResourceLoader::GetInstance().GetModel(modelID));
}

Actor::~Actor()
{
	//処理なし
}

void Actor::ApplyMatrix(const Vector3& scale, const Vector3& pos,
	Quaternion& rotation, int modelHandle)
{
	Matrix4x4 mat;
	//拡大縮小行列
	Matrix4x4 scaleMat = Matrix4x4::Scale(scale);
	mat = scaleMat;
	//回転行列
	//m_rotationをMatrix4x4に変換
	Matrix4x4 rotMat = rotation.ToMatrix4x4();
	mat = mat * rotMat;//合成
	//移動行列
	Matrix4x4 transMat = Matrix4x4::Translate(pos);
	mat = mat * transMat;//合成
	//モデルに適用
	MV1SetMatrix(modelHandle, mat.ToDxLib());
}

void Actor::CreateShaderBuffers()
{
	//定数バッファを作成
	m_cbufferMatrix = CreateShaderConstantBuffer(sizeof(MatrixBuffer));
	m_pCbufferMatrixData = static_cast<MatrixBuffer*>(GetBufferShaderConstantBuffer(m_cbufferMatrix));
	m_cbufferCamera = CreateShaderConstantBuffer(sizeof(CameraBuffer));
	m_pCbufferCameraData = static_cast<CameraBuffer*>(GetBufferShaderConstantBuffer(m_cbufferCamera));
}

void Actor::UpdateShaderMatrixData(const Vector3& cameraPos)
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

void Actor::BindShaderBuffers()
{
	SetShaderConstantBuffer(m_cbufferMatrix, DX_SHADERTYPE_VERTEX, 2);
	SetShaderConstantBuffer(m_cbufferCamera, DX_SHADERTYPE_VERTEX, 3);
	SetShaderConstantBuffer(m_cbufferCamera, DX_SHADERTYPE_PIXEL, 3);
}

void Actor::ReleaseShaderBuffers()
{
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_VERTEX, 2);
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_VERTEX, 3);
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, 3);
}