#include <algorithm>
#include <cassert>

#include "Player.h"
#include "Utility/Quaternion.h"
#include "Manager/LightingManager.h"
#include "Game/GameObjects/Camera/CameraBase.h"
#include "Manager/InputManager.h"
#include "Manager/ResourceLoader.h"
#include "Charactor/Player/Movement/IdleMovementState.h"
#include "Charactor/Player/Rotation/DefaultRotationState.h"
#include "Charactor/Player/Shoot/NormalShootState.h"
#include "SpecialAction/NoneState.h"
#include "Movement/DisabledMovementState.h"
#include "Rotation/DisabledRotState.h"
#include "SpecialAction/SomersaultState.h"
#include "Movement/BoostState.h"
#include "Movement/BrakeState.h"

namespace
{
	//モデルのサイズ
	const Vector3 model_scale = { 0.3f,0.3f,0.3f };

	//移動制限範囲
	constexpr float move_limit_x = 500.0f;
	constexpr float move_limit_y = 300.0f;

	//ゲージの毎フレームの回復量
	constexpr float gauge_recovery_amount = 0.5f;
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
			std::static_pointer_cast<Player>(shared_from_this())
		);

	//RotationStateの初期化
	//通常回転
	m_pRotationState = 
		std::make_shared<DefaultRotationState>(
			std::static_pointer_cast<Player>(shared_from_this())
		);

	//ShootStateの初期化
	//通常弾
	m_pShootState =
		std::make_shared<NormalShootState>(
			std::static_pointer_cast<Player>(shared_from_this()),
			m_pBulletManager);

	//SpecialActionの初期化
	m_pSpecialState =
		std::make_shared<NoneState>(
			std::static_pointer_cast<Player>(shared_from_this()));
}

void Player::Update()
{
	InputManager& input = InputManager::GetInstance();
	//宙返り入力
	Somersault(input);
	//ブーストとブレーキの入力
	Boost(input);
	Brake(input);

	//更新前のSpecialActionStateを保存
	std::shared_ptr<ISpecialActionState> beforeSpecialState = m_pSpecialState;

	//移動系処理の更新処理
	UpdateState(m_pMovementState);
	//回転系処理の更新処理
	UpdateState(m_pRotationState);
	//弾撃ち系処理の更新処理
	UpdateState(m_pShootState);
	//特殊行動系処理の更新処理
	UpdateState(m_pSpecialState);

	//ゲージ使用していないときはゲージを回復する
	if (!IsUseGauge())
	{
		ChangeGauge(gauge_recovery_amount);
	}

	//m_pSpecialStateが切り替わったかどうかを確認
	if (m_pSpecialState != beforeSpecialState)
	{
		//切り替わっていたら通常のステートに戻す
		//通常のステート
		std::shared_ptr<IMovementState> newMoveState =
			std::make_shared<IdleMovementState>(
			std::static_pointer_cast<Player>(shared_from_this()));
		ChangeMovementState(newMoveState);

		//通常のステート
		std::shared_ptr<IRotationState> newRotState =
			std::make_shared<DefaultRotationState>(
			std::static_pointer_cast<Player>(shared_from_this()));
		ChangeRotationState(newRotState);

		//通常のステート
		std::shared_ptr<ISpecialActionState> newSpecialState =
			std::make_shared<NoneState>(
			std::static_pointer_cast<Player>(shared_from_this()));
		ChangeSpecialState(newSpecialState);
	}

	//キャラクターの更新処理
	Charactor::Update();
	//位置をクランプする
	ClampPosition();
}

void Player::ClampPosition()
{
	//移動範囲を制限する
	/*m_pos.m_x = std::clamp(m_pos.m_x, -move_limit_x, move_limit_x);
	m_pos.m_y = std::clamp(m_pos.m_y, -move_limit_y, move_limit_y);*/
}

void Player::Somersault(InputManager& input)
{
	//宙返りの処理
	//左スティックの値を取得して-1～1にする
	Vector2 stick = {
		static_cast<float>(input.GetBufX()) / 1000.0f,
		static_cast<float>(input.GetBufY()) / 1000.0f
	};

	//宙返りボタンが押されていたらステートをそれぞれ切り替える
	if (input.IsTriggered("somersault") &&
		stick.m_y < -0.2f)
	{
		//射撃のみできるようにする
		//全ての入った時の処理も呼ぶ
		//何もしないステート
		std::shared_ptr<IMovementState> newMoveState =
			std::make_shared<DisabledMovementState>(
			std::static_pointer_cast<Player>(shared_from_this()));
		ChangeMovementState(newMoveState);

		//何もしないステート
		std::shared_ptr<IRotationState> newRotState =
			std::make_shared<DisabledRotState>(
			std::static_pointer_cast<Player>(shared_from_this()));
		ChangeRotationState(newRotState);

		//宙返りステートに変更
		std::shared_ptr<ISpecialActionState> newSpecialState = 
			std::make_shared<SomersaultState>(
			std::static_pointer_cast<Player>(shared_from_this()));
		ChangeSpecialState(newSpecialState);
	}
}

void Player::Boost(const InputManager& input)
{
	//ゲージマックス中にブースト入力されたら
	if (input.IsTriggered("boost") &&
		m_gauge >= 100.0f)
	{
		//移動ステートをブースト状態に変更
		std::shared_ptr<IMovementState> newState =
			std::make_shared<BoostState>(
			std::static_pointer_cast<Player>(shared_from_this()));
		//初期化
		ChangeMovementState(newState);
	}
}

void Player::Brake(const InputManager & input)
{
	//ゲージマックス中にブレーキ入力されたら
	if (input.IsTriggered("brake") &&
		m_gauge >= 100.0f)
	{
		//移動ステートをブレーキ状態に変更
		std::shared_ptr<IMovementState> newState =
			std::make_shared<BrakeState>(
			std::static_pointer_cast<Player>(shared_from_this()));
		//初期化
		ChangeMovementState(newState);
	}
}

void Player::ChangeMovementState(std::shared_ptr<IMovementState>(newState))
{
	//前のステートの出るときの処理
	//新しいステートの代入と入るときの処理
	m_pMovementState->Exit();
	m_pMovementState = newState;
	newState->Enter();
}

void Player::ChangeRotationState(std::shared_ptr<IRotationState>(newState))
{
	//前のステートの出るときの処理
	//新しいステートの代入と入るときの処理
	m_pRotationState->Exit();
	m_pRotationState = newState;
	newState->Enter();
}

void Player::ChangeSpecialState(std::shared_ptr<ISpecialActionState>(newState))
{
	//前のステートの出るときの処理
	//新しいステートの代入と入るときの処理
	m_pSpecialState->Exit();
	m_pSpecialState = newState;
	newState->Enter();
}

void Player::Draw()
{
	//モデルに行列を適用
	ApplyMatrix(model_scale,m_pos,m_rotation,m_modelHandle);

	//シェーダに渡すバッファに行列情報を渡す
	SetCbuffMatrixData();

	//シェーダを適用したプレイヤーを描画
	DrawPlayer();

#ifdef _DEBUG
	DrawFormatString(0, 300, 0xffffff, L"playerPosY : %f", m_pos.m_y);
	DrawFormatString(0, 250, 0xffffff, L"Gauge : %f", m_gauge);
#endif
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
	//メタリックマップを取得
	const int metalicGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::PlayerMetalicMap);
	//エミッションマップを取得
	const int emissionGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::PlayerEmissionMap);

#if TRUE
	//法線マップをシェーダに渡す
	//SetUseTextureToShader(1, normGraphH);
	//メタリックマップを渡す
	SetUseTextureToShader(2, metalicGraphH);
	//エミッションマップを渡す
	SetUseTextureToShader(3, emissionGraphH);

	LightingManager::GetInstance().ApplyShader();
	SetShaderConstantBuffer(m_cbufferMatrix, DX_SHADERTYPE_VERTEX, 2);
	SetShaderConstantBuffer(m_cbufferCamera, DX_SHADERTYPE_VERTEX, 3);
	SetShaderConstantBuffer(m_cbufferCamera, DX_SHADERTYPE_PIXEL, 3);
#endif
	//プレイヤーのモデルを描画する
	MV1DrawModel(m_modelHandle);

	////法線マップをシェーダに渡す
	//SetUseTextureToShader(1, -1);//法線マップを解除
	SetUseTextureToShader(2, -1);//メタリックマップを解除
	SetUseTextureToShader(3, -1);//エミッションマップを解除
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
	std::shared_ptr<CameraBase> camera = m_pCamera.lock();
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

void Player::ChangeGauge(float delta)
{
	//増減量を足す
	m_gauge += delta;
	//0～100にクランプ
	m_gauge = std::clamp(m_gauge, 0.0f, 100.0f);
}

void Player::StartUseGauge()
{
	//ゲージ使用を開始
	m_isUseGauge = true;
}

void Player::EndUseGauge()
{
	//ゲージ使用を中止
	m_isUseGauge = false;
}

bool Player::IsUseGauge() const
{
	return m_isUseGauge;
}

void Player::UpdateRotation()
{
	//XとYの回転角からQuaternionを生成
	Quaternion rotX = Quaternion(Vector3(1.0f, 0.0f, 0.0f), m_rotationX);
	Quaternion rotY = Quaternion(Vector3(0.0f, 1.0f, 0.0f), m_rotationY);
	//掛け合わせたものをrotationとする
	m_rotation = rotX * rotY;
}
