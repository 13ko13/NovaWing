#define NOMINMAX
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
#include "Manager/TargetManager.h"
#include "Constants/ShaderRegister.h"
#include "Charactor/Player/Shoot/ChargeReadyState.h"
#include "Constants/Game.h"

namespace
{
	//モデルのサイズ
	const Vector3 model_scale = { 0.3f,0.3f,0.3f };

	//移動制限範囲
	constexpr float move_limit_x = 500.0f;
	constexpr float move_limit_y = 300.0f;

	//ゲージの毎フレームの回復量
	constexpr float gauge_recovery_amount = 0.5f;
	//ゲージの最大値
	constexpr float gauge_max = 100.0f;

	//アナログスティックの入力値を-1～1に正規化するための割る数
	constexpr float stick_input_max = 1000.0f;
	//宙返りの入力と判定するスティック下方向のしきい値
	constexpr float somersault_stick_threshold = -0.2f;

	//当たり判定の球の半径
	constexpr float coll_sphere_radius = 50.0f;
	//当たり判定位置のオフセット
	const Vector3 coll_sphere_offset = { 0.0f, 0.0f, -90.0f };

	//初期座標
	const Vector3 first_pos = { 0.0f,500.0f,0.0f };
}

Player::Player(
	std::shared_ptr<BulletManager> bulletManager,
		ResourceLoader::ModelID modelID,
		std::weak_ptr<CameraBase> camera) :
	Charactor(modelID,camera),
	m_pBulletManager(bulletManager),
	m_collSphere(m_pos)
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

	//初期座標
	m_pos = first_pos;
	//ゲージ初期化
	m_gauge = 100.0f;

	//シェーダに渡す定数バッファを作成
	CreateShaderBuffers();

	//ライトの方向ベクトルをセットする
	LightingManager::GetInstance().SetLightDirection(Vector3(0.0f, -0.5f, -1.0f));

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
			std::static_pointer_cast<Player>(shared_from_this())
		);
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
	//当たり判定の球の位置更新
	//ちょっとずれているのでオフセットで修正
	Vector3 collPos = m_pos + coll_sphere_offset;
	m_collSphere.Update(collPos, coll_sphere_radius);
}

void Player::ClampPosition()
{
	//視錐台クランプを行う
	//カメラをshared_ptrに変換
	std::shared_ptr<CameraBase> pCamera = m_pCamera.lock();
	//今どれくらいの範囲が画面に映っているかを数値として求めるために
	//カメラとプレイヤーの位置の差からZ方向の距離を求める
	Vector3 cameraPos = pCamera->GetPos();//カメラ位置
	Vector3 playerPos = m_pos;
	float distZ = std::abs((playerPos - cameraPos).m_z);
	
	//求めたdistZを使用してプレイヤーが移動できる範囲を
	//ワールド座標で算出する
	Vector2 frustumHalf = pCamera->GetFrustumHalfSize(distZ);

	//プレイヤーの位置をそれぞれ求めた範囲でクランプする
	//-screenWToWorld～screenWToWorldがクランプ範囲
	m_pos.m_x = std::clamp(m_pos.m_x,-frustumHalf.m_x,frustumHalf.m_x);
	//海面と、視錐台の下限を比べて制限が厳しい方を実際の下限として使用する
	m_pos.m_y = std::clamp(
		m_pos.m_y,
		std::max(-frustumHalf.m_y,Game::sea_player_margin),
		frustumHalf.m_y);
}

void Player::Somersault(InputManager& input)
{
	//宙返りの処理
	//左スティックの値を取得して-1～1にする
	Vector2 stick = {
		static_cast<float>(input.GetBufX()) / stick_input_max,
		static_cast<float>(input.GetBufY()) / stick_input_max
	};

	//宙返りボタンが押されていたらステートをそれぞれ切り替える
	if (input.IsTriggered(InputEvent::somersault) &&
		stick.m_y < somersault_stick_threshold)
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
	if (input.IsTriggered(InputEvent::boost) &&
		m_gauge >= gauge_max)
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
	if (input.IsTriggered(InputEvent::brake) &&
		m_gauge >= gauge_max)
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
	UpdateShaderMatrixData();

	//シェーダを適用したプレイヤーを描画
	DrawPlayer();

#ifdef _DEBUG
	DrawFormatString(0, 300, 0xffffff, L"playerPosX:%f,Y : %f,Z:%f", m_pos.m_x, m_pos.m_y, m_pos.m_z);
	DrawFormatString(0, 250, 0xffffff, L"Gauge : %f", m_gauge);
	DrawFormatString(1080, 20, 0xffffff, L"Health : %d", m_health);
	DrawFormatString(0, 365, 0xffffff, L"IsFocus : %d", IsFocus());

	//当たり判定の球を描画
	m_collSphere.Draw(0xff0000);
#endif
}

void Player::DrawPlayer()
{
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

	//法線マップをシェーダに渡す
	SetUseTextureToShader(ShaderRegister::tex_normal, normGraphH);
	//メタリックマップを渡す
	SetUseTextureToShader(ShaderRegister::tex_metalic, metalicGraphH);
	//エミッションマップを渡す
	SetUseTextureToShader(ShaderRegister::tex_emission, emissionGraphH);

	LightingManager::GetInstance().ApplyShader();
	BindShaderBuffers();
	
	//プレイヤーのモデルを描画する
	MV1DrawModel(m_modelHandle);

	SetUseTextureToShader(ShaderRegister::tex_normal, -1);//法線マップを解除
	SetUseTextureToShader(ShaderRegister::tex_metalic, -1);//メタリックマップを解除
	SetUseTextureToShader(ShaderRegister::tex_emission, -1);//エミッションマップを解除
	//シェーダを解除
	LightingManager::GetInstance().ResetShader();
	ReleaseShaderBuffers();
}

void Player::TakeDamage(int damage)
{
	//HPを減らす
	m_health -= damage;
	//HP0以下になったら脂肪処理を行う
	if (m_health <= 0)
	{
		OnDead();
	}
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
	//0～ゲージ最大値にクランプ
	m_gauge = std::clamp(m_gauge, 0.0f, gauge_max);
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

std::weak_ptr<GameObject> Player::GetFocusTarget() const
{
	//ターゲットマネージャーのターゲットを取得してそれを返す
	return m_pTargetManager.lock()->GetFocusTarget();
}

bool Player::IsFocus() const
{
	//フォーカス中か
	return m_pTargetManager.lock()->IsFocus();
}

bool Player::IsChargeReady() const
{
	//m_pShootStateをChargeReadyStateに
	//ダイナミックキャストしてnullじゃなければtrueを返す
	return std::dynamic_pointer_cast<ChargeReadyState>(m_pShootState) != nullptr;
}

void Player::UpdateRotation()
{
	//XとYの回転角からQuaternionを生成
	Quaternion rotX = Quaternion(Vector3(1.0f, 0.0f, 0.0f), m_rotationX);
	Quaternion rotY = Quaternion(Vector3(0.0f, 1.0f, 0.0f), m_rotationY);
	//掛け合わせたものをrotationとする
	m_rotation = rotX * rotY;
}
