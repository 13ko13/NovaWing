#include <algorithm>
#include <cassert>

#include "Player.h"
#include "IPlayerState.h"
#include "Utility/SmartPointerHelper.h"
#include "Utility/Quaternion.h"
#include "Utility/Matrix4x4.h"
#include "LightingManager.h"
#include "Game/GameObjects/Camera/CameraBase.h"
#include "Utility/SmartPointerHelper.h"
#include "Manager/InputManager.h"
#include "Manager/ResourceLoader.h"
#include "IdleMovementState.h"
#include "DefaultRotationState.h"

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

	//MovementStateの初期化
	//待機状態
	m_pMovementState = 
		std::make_shared<IdleMovementState>(
			GameObjectToDerived<Player>(shared_from_this())
		);

	//RotationStateの初期化
	//通常回転
	m_pRotationState = 
		std::make_shared<DefaultRotationState>(
			GameObjectToDerived<Player>(shared_from_this())
		);
}

void Player::Update()
{
	//移動系処理の更新処理
	m_pMovementState->Update();
	//回転系処理の更新処理
	m_pRotationState->Update();

	//キャラクターの更新処理
	Charactor::Update();

	ClampPosition();

	//次のステートを取得
	std::shared_ptr<IMovementState> pMoveNextState =
		m_pMovementState->GetNextState();

	//次のステートを確認して、nullじゃなければExitを呼ぶ
	if (pMoveNextState != nullptr)
	{
		//ステートを抜けるときの処理
		m_pMovementState->Exit();
		//ステートを差し替える
		m_pMovementState = pMoveNextState;
		//差し替えた後のステートの入るときの処理を呼ぶ
		m_pMovementState->Enter();
	}

	//次のステートを取得
	std::shared_ptr<IRotationState> pRotNextState =
		m_pRotationState->GetNextState();

	//次のステートを確認して、nullじゃなければExitを呼ぶ
	if (pRotNextState != nullptr)
	{
		//ステートを抜けるときの処理
		m_pRotationState->Exit();
		//ステートを差し替える
		m_pRotationState = pRotNextState;
		//差し替えた後のステートの入るときの処理を呼ぶ
		m_pRotationState->Enter();
	}
}

void Player::ClampPosition()
{
	//移動範囲を制限する
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

void Player::LerpToAngleX(float targetAngle, float t)
{
	//targetAngleに向けてrotationXをLerpする
	m_rotationX = m_rotationX * (1 - t) + targetAngle * t;
	//Rotationを適用する
	UpdateRotation();
}

void Player::LerpToAngleY(float targetAngle, float t)
{
	//targetAngleに向けてrotationYをLerpする
	m_rotationY = m_rotationY * (1 - t) + targetAngle * t;
	//Rotationを適用する
	UpdateRotation();
}

void Player::UpdateRotation()
{
	//XとYの回転角からQuaternionを生成
	Quaternion rotX = Quaternion(Vector3(1.0f, 0.0f, 0.0f), m_rotationX);
	Quaternion rotY = Quaternion(Vector3(0.0f, 1.0f, 0.0f), m_rotationY);
	//掛け合わせたものをrotationとする
	m_rotation = rotX * rotY;
}
