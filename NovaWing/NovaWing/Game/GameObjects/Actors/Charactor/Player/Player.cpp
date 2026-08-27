#define NOMINMAX
#include <algorithm>
#include <cassert>
#include <cmath>

#include "Charactor/Player/Movement/IdleMovementState.h"
#include "Charactor/Player/Rotation/DefaultRotationState.h"
#include "Charactor/Player/Shoot/ChargeReadyState.h"
#include "Charactor/Player/Shoot/ChargeShootState.h"
#include "Charactor/Player/Shoot/NormalShootState.h"
#include "Constants/Game.h"
#include "Constants/ShaderRegister.h"
#include "Game/GameObjects/Camera/CameraBase.h"
#include "Manager/InputManager.h"
#include "Manager/LightingManager.h"
#include "Manager/ResourceLoader.h"
#include "Manager/TargetManager.h"
#include "Movement/BoostState.h"
#include "Movement/BrakeState.h"
#include "Movement/DisabledMovementState.h"
#include "Player.h"
#include "Rotation/DisabledRotState.h"
#include "SpecialAction/NoneState.h"
#include "SpecialAction/SomersaultState.h"
#include "Utility/Quaternion.h"
#include "Shoot/DisabledShootState.h"

namespace
{
	// モデルのサイズ
	const Vector3 model_scale = {0.3f, 0.3f, 0.3f};

	// ゲージの毎フレームの回復量
	constexpr float gauge_recovery_amount = 0.5f;
	// ゲージの最大値
	constexpr float gauge_max = 100.0f;

	// アナログスティックの入力値を-1～1に正規化するための割る数
	constexpr float stick_input_max = 1000.0f;
	// 宙返りの入力と判定するスティック上方向のしきい値
	constexpr float somersault_stick_threshold = -0.2f;

	// 当たり判定の球の半径
	constexpr float coll_sphere_radius = 50.0f;
	// 当たり判定位置のオフセット
	const Vector3 coll_sphere_offset = {0.0f, 0.0f, -90.0f};

	// 初期座標
	const Vector3 first_pos = {0.0f, 500.0f, 1200.0f};

	// HPの最大値
	constexpr int max_health = 100;

	//視錐台クランプの際に視錐台が広まりきる前に
	//クランプされていたので、カメラからプレイヤーまでの距離を測るのではなく
	//別で定数を用意する
	constexpr float clamp_frustum_distz = 900.0f;

	//ダメージエフェクトをどれぐらいのフレーム間で出すか
	constexpr float damage_eff_frame = 30.0f;

	//ダメージシェーダのハンドル
	constexpr const wchar_t* damage_shader_path = L"DamagePS.pso";

	//ゲージの初期値
	constexpr float first_gauge = 100.0f;
} // namespace

Player::Player(
	std::shared_ptr<BulletManager> bulletManager,
	ResourceLoader::ModelID modelID,
	std::weak_ptr<CameraBase> camera,
	std::weak_ptr<SoundManager> soundManager) :
	Charactor(modelID, camera),
	m_pBulletManager(bulletManager),
	m_pSoundManager(soundManager),
	m_collSphere(m_pos)
{
}

Player::~Player()
{
	// 処理なし
}

void Player::OnInit()
{
	// Y軸に180度回転する(モデルが反対を向いているので)
	Vector3 axis = Vector3(0.0f, 1.0f, 0.0f);
	m_rotationY = DX_PI_F;
	UpdateRotation();

	// 初期座標
	m_pos = first_pos;
	// ゲージ初期化
	m_gauge = first_gauge;

	// シェーダに渡す定数バッファを作成
	CreateShaderBuffers();

	// MovementStateの初期化
	// 待機状態
	m_pMovementState =
		std::make_shared<IdleMovementState>(
			std::static_pointer_cast<Player>(shared_from_this()));

	// RotationStateの初期化
	// 通常回転
	m_pRotationState =
		std::make_shared<DefaultRotationState>(
			std::static_pointer_cast<Player>(shared_from_this()));

	// ShootStateの初期化
	// 通常弾
	m_pShootState =
		std::make_shared<NormalShootState>(
			std::static_pointer_cast<Player>(shared_from_this()),
			m_pBulletManager,
			m_pSoundManager);

	// SpecialActionの初期化
	m_pSpecialState =
		std::make_shared<NoneState>(
			std::static_pointer_cast<Player>(shared_from_this()));

	//ダメージバッファを作成
	m_cbufferDamage = CreateShaderConstantBuffer(sizeof(DamageBuffer));
	m_pCBufferDamageData = static_cast<DamageBuffer*>(GetBufferShaderConstantBuffer(m_cbufferDamage));

	//ダメージシェーダのロード
	m_damageShaderPSH = LoadPixelShader(damage_shader_path);
}

void Player::Update()
{
	//ダメージエフェクト許可されている間にエフェクトの値計算
	if (m_isDamageEffect)
	{
		m_damageTime++;
		//sinfの結果が-1までいかないように180
		float angle = 180.0f * (static_cast<float>(m_damageTime) / damage_eff_frame);
		m_pCBufferDamageData->redAmount = sinf(angle * DX_PI_F / 180.0f); 
		//ダメージエフェクトを出さないようにする
		if (m_damageTime > damage_eff_frame)
		{
			m_isDamageEffect = false;
			m_damageTime = 0;
		}
	}
	//定数バッファを更新
	UpdateShaderConstantBuffer(m_cbufferDamage);

	InputManager& input = InputManager::GetInstance();
	// 宙返り入力
	Somersault(input);
	// ブーストとブレーキの入力
	Boost(input);
	Brake(input);

	// 更新前のSpecialActionStateを保存
	std::shared_ptr<ISpecialActionState> beforeSpecialState = m_pSpecialState;

	// 移動系処理の更新処理
	UpdateState(m_pMovementState);
	// 回転系処理の更新処理
	UpdateState(m_pRotationState);
	// 弾撃ち系処理の更新処理
	UpdateState(m_pShootState);
	// 特殊行動系処理の更新処理
	UpdateState(m_pSpecialState);

	// ゲージ使用していないときはゲージを回復する
	if (!IsUseGauge())
	{
		ChangeGauge(gauge_recovery_amount);
	}

	// m_pSpecialStateが切り替わったかどうかを確認
	if (m_pSpecialState != beforeSpecialState)
	{
		// 切り替わっていたら通常のステートに戻す
		ChangeAllStateToNormal();
	}

	// キャラクターの更新処理
	Charactor::Update();
	// 位置をクランプする
	ClampPosition();
	// 当たり判定の球の位置更新
	// ちょっとずれているのでオフセットで修正
	Vector3 collPos = m_pos + coll_sphere_offset;
	m_collSphere.Update(collPos, coll_sphere_radius);

#ifdef _DEBUG
	// ボタンでゲージを減らしたり増やしたりできるようにする
	if (input.IsPressed(InputEvent::gaugeUp))
	{
		m_health++;
	}
	else if (input.IsPressed(InputEvent::gaugeDown))
	{
		m_health--;
	}
	//Wキーでボスまでワープ
	if (input.IsPressed(InputEvent::bossWarp))
	{
		m_pos.m_z = 19000.0f;
	}

#endif
	// HPのクランプを行う
	m_health = std::clamp(m_health, 0, max_health);
}

void Player::ClampPosition()
{
	// 宙返り中はクランプしてほしくないので処理を飛ばす
	if (IsSomersault()) return;

	// 視錐台クランプを行う
	// カメラをshared_ptrに変換
	std::shared_ptr<CameraBase> pCamera = m_pCamera.lock();
	// 今どれくらいの範囲が画面に映っているかを数値として求めるために
	// カメラとプレイヤーの位置の差からZ方向の距離を求める
	Vector3 cameraPos = pCamera->GetPos(); // カメラ位置
	Vector3 playerPos = m_pos;
	float distZ = std::abs((playerPos - cameraPos).m_z);

	// 求めたdistZを使用してプレイヤーが移動できる範囲を
	// ワールド座標で算出する
	Vector2 frustumHalf = pCamera->GetFrustumHalfSize(clamp_frustum_distz);

	// プレイヤーの位置をそれぞれ求めた範囲でクランプする
	//-screenWToWorld～screenWToWorldがクランプ範囲
	m_pos.m_x = std::clamp(m_pos.m_x, -frustumHalf.m_x, frustumHalf.m_x);
	// 海面と、視錐台の下限を比べて制限が厳しい方を実際の下限として使用する
	m_pos.m_y = std::clamp(
		m_pos.m_y,
		std::max(-frustumHalf.m_y, Game::sea_player_margin),
		frustumHalf.m_y);
}

void Player::Somersault(InputManager& input)
{
	// 宙返りの処理
	// 左スティックの値を取得して-1～1にする
	Vector2 stick = {
		static_cast<float>(input.GetBufX()) / stick_input_max,
		static_cast<float>(input.GetBufY()) / stick_input_max};

	// 宙返りボタンが押されていたらステートをそれぞれ切り替える
	if (input.IsTriggered(InputEvent::somersault) &&
		stick.m_y < somersault_stick_threshold)
	{
		// 射撃のみできるようにする
		// 全ての入った時の処理も呼ぶ
		// 何もしないステート
		std::shared_ptr<IMovementState> newMoveState =
			std::make_shared<DisabledMovementState>(
				std::static_pointer_cast<Player>(shared_from_this()));
		ChangeMovementState(newMoveState);

		// 何もしないステート
		std::shared_ptr<IRotationState> newRotState =
			std::make_shared<DisabledRotState>(
				std::static_pointer_cast<Player>(shared_from_this()));
		ChangeRotationState(newRotState);

		// 宙返りステートに変更
		std::shared_ptr<ISpecialActionState> newSpecialState =
			std::make_shared<SomersaultState>(
				std::static_pointer_cast<Player>(shared_from_this()),
				m_pSoundManager);
		ChangeSpecialState(newSpecialState);
	}
}

void Player::Boost(const InputManager& input)
{
	// ゲージマックス中にブースト入力されたら
	if (input.IsTriggered(InputEvent::boost) &&
		m_gauge >= gauge_max)
	{
		// 移動ステートをブースト状態に変更
		std::shared_ptr<IMovementState> newState =
			std::make_shared<BoostState>(
				std::static_pointer_cast<Player>(shared_from_this()),
				m_pSoundManager);
		// 初期化
		ChangeMovementState(newState);
	}
}

void Player::Brake(const InputManager& input)
{
	// ゲージマックス中にブレーキ入力されたら
	if (input.IsTriggered(InputEvent::brake) &&
		m_gauge >= gauge_max)
	{
		// 移動ステートをブレーキ状態に変更
		std::shared_ptr<IMovementState> newState =
			std::make_shared<BrakeState>(
				std::static_pointer_cast<Player>(shared_from_this()),
				m_pSoundManager);
		// 初期化
		ChangeMovementState(newState);
	}
}

void Player::ChangeMovementState(std::shared_ptr<IMovementState>(newState))
{
	// 前のステートの出るときの処理
	// 新しいステートの代入と入るときの処理
	m_pMovementState->Exit();
	m_pMovementState = newState;
	newState->Enter();
}

void Player::ChangeShootState(std::shared_ptr<IShootState>(newState))
{
	//前のステートの出るときの処理
	//新しいステートの代入と入るときの処理
	m_pShootState->Exit();
	m_pShootState = newState;
	newState->Enter();
}

void Player::ChangeRotationState(std::shared_ptr<IRotationState>(newState))
{
	// 前のステートの出るときの処理
	// 新しいステートの代入と入るときの処理
	m_pRotationState->Exit();
	m_pRotationState = newState;
	newState->Enter();
}

void Player::ChangeSpecialState(std::shared_ptr<ISpecialActionState>(newState))
{
	// 前のステートの出るときの処理
	// 新しいステートの代入と入るときの処理
	m_pSpecialState->Exit();
	m_pSpecialState = newState;
	newState->Enter();
}

void Player::Draw()
{
	// モデルに行列を適用
	ApplyMatrix(model_scale, m_pos, m_rotation, m_modelHandle);

	// シェーダに渡すバッファに行列情報を渡す
	UpdateShaderMatrixData();

	// シェーダを適用したプレイヤーを描画
	DrawPlayer();

#ifdef _DEBUG
	DrawFormatString(0, 300, 0xffffff, L"playerPosX:%f,Y : %f,Z:%f", m_pos.m_x, m_pos.m_y, m_pos.m_z);
	DrawFormatString(0, 250, 0xffffff, L"Gauge : %f", m_gauge);
	DrawFormatString(1080, 20, 0xffffff, L"Health : %d", m_health);
	DrawFormatString(0, 365, 0xffffff, L"IsFocus : %d", IsFocus());

	// 当たり判定の球を描画
	m_collSphere.Draw(0xff0000);
#endif
}

void Player::DrawPlayer()
{
	// ResourceLoaderからPlayerの法線マップを取得
	// ResourceLoaderのインスタンスを取得
	const ResourceLoader& resourceLoader = ResourceLoader::GetInstance();
	// 法線マップ取得
	const int normGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::PlayerNormalMap);
	// メタリックマップを取得
	const int metalicGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::PlayerMetalicMap);
	// エミッションマップを取得
	const int emissionGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::PlayerEmissionMap);

	// 法線マップをシェーダに渡す
	SetUseTextureToShader(ShaderRegister::tex_normal, normGraphH);
	// メタリックマップを渡す
	SetUseTextureToShader(ShaderRegister::tex_metalic, metalicGraphH);
	// エミッションマップを渡す
	SetUseTextureToShader(ShaderRegister::tex_emission, emissionGraphH);

	LightingManager::GetInstance().ApplyShader();
	BindShaderBuffers();

	if (m_isDamageEffect)
	{
		//ダメージシェーダに定数バッファをセットする
		SetShaderConstantBuffer(m_cbufferDamage, DX_SHADERTYPE_PIXEL, ShaderRegister::cbuffer_damage);
		SetUsePixelShader(m_damageShaderPSH);
	}

	// プレイヤーのモデルを描画する
	MV1DrawModel(m_modelHandle);

	SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, ShaderRegister::cbuffer_damage);

	SetUseTextureToShader(ShaderRegister::tex_normal, -1);	 // 法線マップを解除
	SetUseTextureToShader(ShaderRegister::tex_metalic, -1);	 // メタリックマップを解除
	SetUseTextureToShader(ShaderRegister::tex_emission, -1); // エミッションマップを解除
	// シェーダを解除
	LightingManager::GetInstance().ResetShader();
	ReleaseShaderBuffers();
	SetUsePixelShader(-1);
}

void Player::TakeDamage(int damage)
{
	// HPを減らす
	m_health -= damage;

	//ダメージエフェクトを出すフラグ
	m_isDamageEffect = true;

	//被弾回数をインクリメント
	m_hitCount++;

	//ダメージ音を鳴らす
	std::shared_ptr<SoundManager> pSoundManager = m_pSoundManager.lock();
	pSoundManager->Play(SoundManager::SoundType::PlayerDamage, false, true);

	// HP0以下になったら死亡処理を行う
	if (m_health <= 0)
	{
		//死亡音を鳴らす
		pSoundManager->Play(SoundManager::SoundType::PlayerDeath,false,true);
		OnDead();
	}
}

void Player::LerpToAngleX(float targetAngle, float t)
{
	// targetAngleに向けてrotationXをLerpする
	m_rotationX = m_rotationX * (1 - t) + targetAngle * t;
	// Rotationを適用する
	UpdateRotation();
}

void Player::LerpToAngleY(float targetAngle, float t)
{
	// targetAngleに向けてrotationYをLerpする
	m_rotationY = m_rotationY * (1 - t) + targetAngle * t;
	// Rotationを適用する
	UpdateRotation();
}

void Player::ChangeGauge(float delta)
{
	// 増減量を足す
	m_gauge += delta;
	// 0～ゲージ最大値にクランプ
	m_gauge = std::clamp(m_gauge, 0.0f, gauge_max);
}

void Player::StartUseGauge()
{
	// ゲージ使用を開始
	m_isUseGauge = true;
}

void Player::EndUseGauge()
{
	// ゲージ使用を中止
	m_isUseGauge = false;
}

bool Player::IsUseGauge() const
{
	return m_isUseGauge;
}

std::weak_ptr<EnemyBase> Player::GetForcusTarget() const
{
	// ターゲットマネージャーのターゲットを取得してそれを返す
	return m_pTargetManager.lock()->GetFocusTarget();
}

bool Player::IsFocus() const
{
	// フォーカス中か
	return m_pTargetManager.lock()->IsFocus();
}

bool Player::IsChargeReady() const
{
	// m_pShootStateをChargeReadyState、ChargeShootStateに
	// あてはめて、nullならチャージ中、チャージ完了中ではないので
	// falseを返す。nullではないということはチャージ中、チャージ完了中ということ
	if (std::dynamic_pointer_cast<ChargeShootState>(m_pShootState) != nullptr)
	{
		return true;
	}
	// ダイナミックキャストしてnullじゃなければtrueを返す
	return std::dynamic_pointer_cast<ChargeReadyState>(m_pShootState) != nullptr;
}

bool Player::IsSomersault() const
{
	// m_pSpecialStateをSomersaultStateに
	// あてはめてnullなら宙返り中ではないことを表す
	// nullではないということは当てはまるので、宙返り中ということを表す
	return std::dynamic_pointer_cast<SomersaultState>(m_pSpecialState) != nullptr;
}

void Player::ChangeAllStateToDisabled()
{
	// 全ての入った時の処理も呼ぶ
	// 何もしないステート
	std::shared_ptr<IMovementState> newMoveState =
		std::make_shared<DisabledMovementState>(
			std::static_pointer_cast<Player>(shared_from_this()));
	ChangeMovementState(newMoveState);

	// 何もしないステート
	std::shared_ptr<IRotationState> newRotState =
		std::make_shared<DisabledRotState>(
			std::static_pointer_cast<Player>(shared_from_this()));
	ChangeRotationState(newRotState);

	//何もしないステート
	std::shared_ptr<IShootState> newShootState =
		std::make_shared<DisabledShootState>(
			std::static_pointer_cast<Player>(shared_from_this()), m_pBulletManager, m_pSoundManager);
	ChangeShootState(newShootState);
}

void Player::ChangeAllStateToNormal()
{
	// 全ての入った時の処理も呼ぶ
	// 通常ステート
	std::shared_ptr<IMovementState> newMoveState =
		std::make_shared<IdleMovementState>(
			std::static_pointer_cast<Player>(shared_from_this()));
	ChangeMovementState(newMoveState);

	// 通常ステート
	std::shared_ptr<IRotationState> newRotState =
		std::make_shared < DefaultRotationState > (
			std::static_pointer_cast<Player>(shared_from_this()));
	ChangeRotationState(newRotState);

	// 通常ステート
	std::shared_ptr<IShootState> newShootState =
		std::make_shared<NormalShootState>(
			std::static_pointer_cast<Player>(shared_from_this()), m_pBulletManager, m_pSoundManager);
	ChangeShootState(newShootState);
}

void Player::UpdateRotation()
{
	// XとYの回転角からQuaternionを生成
	Quaternion rotX = Quaternion(Vector3(1.0f, 0.0f, 0.0f), m_rotationX);
	Quaternion rotY = Quaternion(Vector3(0.0f, 1.0f, 0.0f), m_rotationY);
	// 掛け合わせたものをrotationとする
	m_rotation = rotX * rotY;
}