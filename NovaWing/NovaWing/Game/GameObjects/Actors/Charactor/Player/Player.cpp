#include <algorithm>
#include <cassert>

#include "Player.h"
#include "IPlayerState.h"
#include "NormalState.h"
#include "../../../../../Utility/SmartPointerHelper.h"
#include "../../../../../Utility/Quaternion.h"
#include "../../../../../Utility/Matrix4x4.h"
#include "../../../../../LightingManager.h"
#include "../../../Camera/CameraBase.h"
#include "../../../../../Utility/SmartPointerHelper.h"
#include "../../../../../Manager/InputManager.h"
#include "../../../../../Manager/ResourceLoader.h"

namespace
{
	//モデルのサイズ
	const Vector3 model_scale = { 0.3f,0.3f,0.3f };

	//移動制限範囲
	constexpr float move_limit_x = 500.0f;
	constexpr float move_limit_y = 300.0f;
}

Player::Player(
	std::shared_ptr<BulletManager> bulletManager,
	ResourceLoader::ModelID modelID) :
	Charactor(modelID),
	m_pBulletManager(bulletManager)
{

}

Player::~Player()
{
	//処理なし
}

void Player::OnInit()
{
	//Normalステートに初期化
	//Nullチェック
	//shared_from_thisがPlayerではなくActor型なのでPlayerにキャストする
	m_pCurrentState = std::make_shared<NormalState>(static_pointer_cast<Player>(shared_from_this()));

	//Y軸に180度回転する(モデルが反対を向いているので)
	Vector3 axis = Vector3(0.0f, 1.0f, 0.0f);
	m_rotationY = DX_PI_F;
	UpdateRotation();

	m_cbufferMatrix = CreateShaderConstantBuffer(sizeof(MatrixBuffer));
	m_pCbufferMatrixData = static_cast<MatrixBuffer*>(GetBufferShaderConstantBuffer(m_cbufferMatrix));

	m_cbufferCamera = CreateShaderConstantBuffer(sizeof(CameraBuffer));
	m_pCbufferCameraData = static_cast<CameraBuffer*>(GetBufferShaderConstantBuffer(m_cbufferCamera));

	//ライトの方向ベクトルをセットする
	LightingManager::GetInstance().SetLightDirection(Vector3(1.0f, -1.0f, -1.0f));
}

void Player::Update()
{
	Charactor::Update();

	//もしプレイヤーの現在のステートが存在するなら
	if (m_pCurrentState)
	{
		m_pCurrentState->Update();
	}
	m_pos.m_x = std::clamp(m_pos.m_x, -move_limit_x, move_limit_x);
	m_pos.m_y = std::clamp(m_pos.m_y, -move_limit_y, move_limit_y);
}

void Player::Draw()
{
	//モデルに行列を適用
	ApplyMatrix();

	//シェーダに渡すバッファに行列情報を渡す
	SetCbuffMatrixData();

	//シェーダを適用したプレイヤーを描画
	DrawPlayer();
}

void Player::ApplyMatrix()
{
	Matrix4x4 mat;
	//拡大縮小行列
	Matrix4x4 scaleMat = Matrix4x4::Scale(model_scale);
	mat = scaleMat;
	//回転行列
	//m_rotationをMatrix4x4に変換
	Matrix4x4 rotMat = m_rotation.ToMatrix4x4();
	mat = mat * rotMat;//合成
	//移動行列
	Matrix4x4 transMat = Matrix4x4::Translate(m_pos);
	mat = mat * transMat;//合成
	//モデルに適用
	MV1SetMatrix(m_modelHandle, mat.ToDxLib());
}

void Player::DrawPlayer()
{
	//シェーダを適用する
	UpdateShaderConstantBuffer(m_cbufferMatrix);
	UpdateShaderConstantBuffer(m_cbufferCamera);

	//ResourceLoaderからPlayerの法線マップを取得
	//ResourceLoaderのインスタンスを取得
	const ResourceLoader& resourceLoader = ResourceLoader::GetInstance();
	//法線マップ取得
	const int normGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::PlayerNormalMap);

#if TRUE
	//法線マップをシェーダに渡す
	SetUseTextureToShader(1, normGraphH);

	LightingManager::GetInstance().ApplyShader();
	SetShaderConstantBuffer(m_cbufferMatrix, DX_SHADERTYPE_VERTEX, 2);
	SetShaderConstantBuffer(m_cbufferCamera, DX_SHADERTYPE_VERTEX, 3);
	SetShaderConstantBuffer(m_cbufferCamera, DX_SHADERTYPE_PIXEL, 3);
#endif
	//プレイヤーのモデルを描画する
	MV1DrawModel(m_modelHandle);

	//法線マップをシェーダに渡す
	SetUseTextureToShader(1, -1);//法線マップを解除
	//シェーダを解除
	LightingManager::GetInstance().ResetShader();
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_VERTEX, 2);
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_VERTEX, 3);
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, 3);
}

void Player::SetCbuffMatrixData()
{
	//行列情報を入れる
	m_pCbufferMatrixData->world = MV1GetLocalWorldMatrix(m_modelHandle);
	m_pCbufferMatrixData->view = GetCameraViewMatrix();
	m_pCbufferMatrixData->proj = GetCameraProjectionMatrix();

	//カメラの位置をMatrix情報に渡す]
	//shared_ptrに変換
	std::shared_ptr<CameraBase> camera = WeakToShared(m_pCamera);
	assert(camera != nullptr);//Nullチェック
	//カメラの位置も入れる
	m_pCbufferCameraData->cameraPos = camera->GetPos();
}

void Player::TakeDamage(int damage)
{

}

void Player::RotateX(float angle)
{
	//angleを加算する
	m_rotationX += angle;
	//角度を制限する
	m_rotationX = std::clamp(m_rotationX, -DX_PI_F / 6.0f, DX_PI_F / 6);
	//回転を適用する
	UpdateRotation();
}

void Player::RotateY(float angle)
{
	//angleを加算する
	m_rotationY += angle;
	//角度を制限する
	m_rotationY = std::clamp(m_rotationY, DX_PI_F - DX_PI_F / 6.0f, DX_PI_F + DX_PI_F / 6.0f);
	//回転を適用する
	UpdateRotation();
}

void Player::LerpRotation(float t)
{
	//0に向けてrotationXをLerpする
	m_rotationX = m_rotationX * (1 - t) + 0 * t;
	//DX_PI_Fに向けてrotationYをLerpする
	m_rotationY = m_rotationY * (1 - t) + DX_PI_F * t;
	//Rotationを適用する
	UpdateRotation();
}

void Player::ChangeState(std::shared_ptr<IPlayerState> pNextState)
{
	//Nullチェック
	if (m_pCurrentState != nullptr)
	{
		//現在のステートの出たときの処理を呼ぶ
		m_pCurrentState->Exit();
	}

	//新しいステートに差し替える
	m_pCurrentState = pNextState;
	//そのステートの入った時の処理を呼ぶ
	m_pCurrentState->Enter();
}

void Player::UpdateRotation()
{
	//XとYの回転角からQuaternionを生成
	Quaternion rotX = Quaternion(Vector3(1.0f, 0.0f, 0.0f), m_rotationX);
	Quaternion rotY = Quaternion(Vector3(0.0f, 1.0f, 0.0f), m_rotationY);
	//掛け合わせたものをrotationとする
	m_rotation = rotX * rotY;
}
