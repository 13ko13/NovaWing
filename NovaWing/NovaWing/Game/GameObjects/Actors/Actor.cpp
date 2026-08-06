#include <DxLib.h>

#include "Actor.h"
#include "Utility/Matrix4x4.h"
#include "Game/GameObjects/Camera/CameraBase.h"
#include "Constants/ShaderRegister.h"
#include "Manager/LightingManager.h"

Actor::Actor(ResourceLoader::ModelID modelID,
	std::weak_ptr<CameraBase> pCamera) :
	m_pCamera(pCamera)
{
	//受け取ったモデルIDをもとにモデルを複製してハンドルを取得する
	m_modelHandle = MV1DuplicateModel(
		ResourceLoader::GetInstance().GetModel(modelID));
	
	//マテリアル単位でブレンド無しを明示する
    //(全体のSetDrawBlendModeはMV1描画時にマテリアル設定で上書きされるため)
	//これをしておかないと、水に埋まっている部分が見えなくなってしまう
    int matNum = MV1GetMaterialNum(m_modelHandle);
    for (int i = 0; i < matNum; i++)
    {
        MV1SetMaterialDrawBlendMode(m_modelHandle, i, DX_BLENDMODE_NOBLEND);
    }
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

void Actor::UpdateShaderMatrixData()
{
	//行列情報を入れる
	m_pCbufferMatrixData->world = MV1GetLocalWorldMatrix(m_modelHandle);
	m_pCbufferMatrixData->view = GetCameraViewMatrix();
	m_pCbufferMatrixData->proj = GetCameraProjectionMatrix();

	//カメラの位置をMatrix情報に渡す
	m_pCbufferCameraData->cameraPos = m_pCamera.lock()->GetPos();


	UpdateShaderConstantBuffer(m_cbufferMatrix);
	UpdateShaderConstantBuffer(m_cbufferCamera);
}

void Actor::BindShaderBuffers()
{
	SetShaderConstantBuffer(m_cbufferMatrix, DX_SHADERTYPE_VERTEX, ShaderRegister::cbuffer_matrix);
	SetShaderConstantBuffer(m_cbufferCamera, DX_SHADERTYPE_VERTEX, ShaderRegister::cbuffer_camera);
	SetShaderConstantBuffer(m_cbufferCamera, DX_SHADERTYPE_PIXEL, ShaderRegister::cbuffer_camera);
}

void Actor::ReleaseShaderBuffers()
{
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_VERTEX, ShaderRegister::cbuffer_matrix);
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_VERTEX, ShaderRegister::cbuffer_camera);
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, ShaderRegister::cbuffer_camera);
}

Vector3 Actor::GetCameraPos() const
{
    //shared_ptrに変換して位置を返す
	return m_pCamera.lock()->GetPos();
}

void Actor::DrawWithLighting(const std::vector<std::pair<int, int>>& textures,
	bool isSkinned)
{
	//テクスチャの中身を1つずつ取り出し、テクスチャレジスタにセット
	for(const std::pair<int,int>& tex : textures)
	{
		//テクスチャをセット
		SetUseTextureToShader(tex.first,tex.second);
	}

	//シェーダを適用
	LightingManager::GetInstance().ApplyShader(isSkinned);
	BindShaderBuffers();//シェーダをセットする
	//モデルを描画
	MV1DrawModel(m_modelHandle);

	//テクスチャを開放する
	for(const std::pair<int,int>& tex : textures)
	{
		//テクスチャを解除
		SetUseTextureToShader(tex.first,-1);
	}
	//シェーダを解除
	LightingManager::GetInstance().ResetShader();
	//開放する
	ReleaseShaderBuffers();
}
