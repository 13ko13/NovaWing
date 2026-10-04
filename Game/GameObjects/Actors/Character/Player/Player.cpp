#define NOMINMAX
#include <algorithm>
#include <cassert>
#include <cmath>

#include "Character/Player/Movement/IdleMovementState.h"
#include "Character/Player/Rotation/DefaultRotationState.h"
#include "Character/Player/Shoot/ChargeReadyState.h"
#include "Character/Player/Shoot/ChargeShootState.h"
#include "Character/Player/Shoot/NormalShootState.h"
#include "Constants/Game.h"
#include "Constants/ShaderRegister.h"
#include "Game/GameObjects/Camera/CameraBase.h"
#include "Manager/InputManager.h"
#include "Manager/DebugManager.h"
#include "Manager/EffectManager.h"
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
#include "Game/Collision/PlayerCollider.h"
#include "Game/Collision/CounterCollider.h"

namespace
{
	// モデルのサイズ
	const Vector3 model_scale = {0.25f, 0.25f, 0.25f};

	// ゲージの毎フレームの回復量
	constexpr float gauge_recovery_amount = 0.5f;
	// ゲージの最大値
	constexpr float gauge_max = 100.0f;

	// アナログスティックの入力値を-1～1に正規化するための割る数
	constexpr float stick_input_max = 1000.0f;
	// 宙返りの入力と判定するスティック上方向のしきい値
	constexpr float somersault_stick_threshold = -0.2f;

	// 当たり判定の球の半径
	constexpr float hit_coll_sphere_radius = 50.0f;
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

	//ゲージの最大値
	constexpr float max_gauge = 100.0f;

	//モデルの羽のボーン名
	constexpr const wchar_t* left_wing_bone_name = L"Collider7";
	constexpr const wchar_t* right_wing_bone_name = L"Collider6";

	//宙返りを使用できるゲージ量
	constexpr float somersoult_use_gauge = 50.0f;

	//海の高さ
	constexpr float sea_height = 30.0f;
	//海面に水しぶきのエフェクトを出す高さ
	constexpr float sea_splash_height_threshold = 200.0f;
	//水しぶきのエフェクトのオフセットX
	constexpr float sea_splash_offset_x = 50.0f;
	//水しぶきのエフェクトのオフセットZ
	constexpr float sea_splash_offset_z = -50.0f;
	
	//カウンター用の球の半径
	constexpr float counter_col_radius = 100.0f;

	//水しぶきのフェードの倍率（大きいほど、しきい値ぎりぎりまで濃いまま急に薄くなる）
	constexpr float sea_splash_fade_scale = 0.75f;

	//海からこの距離以内であれば回転を不可能にする
	constexpr float sea_roll_limit_height = 150.0f;
} // namespace

Player::Player(
	std::shared_ptr<BulletManager> bulletManager,
	ResourceLoader::ModelID modelID,
	std::weak_ptr<CameraBase> camera,
	std::weak_ptr<SoundManager> soundManager,
	std::weak_ptr<EffectManager> effectManager) :
	Character(modelID, camera),
	m_pBulletManager(bulletManager),
	m_pSoundManager(soundManager),
	m_pEffectManager(effectManager)
{
	m_pHitCollider = std::make_unique<PlayerCollider>(*this);
	m_pCounterCollider = std::make_unique<CounterCollider>(*this);
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
	m_gauge = max_gauge;

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
			m_pSoundManager,
			m_pTargetManager);

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
	Character::Update();
	// 位置をクランプする
	ClampPosition();
	// 当たり判定の球の位置更新
	// ちょっとずれているのでオフセットで修正
	Vector3 collPos = m_pos + coll_sphere_offset;
	m_pHitCollider->UpdateShape(collPos, hit_coll_sphere_radius);
	//カウンターの球も更新
	m_pCounterCollider->UpdateShape(collPos, counter_col_radius);

	//プレイヤーが海すれすれにいたら、羽の位置を基準に
	//海面に水しぶきのエフェクトを出す
	//プレイヤーの羽のボーン位置を取得
	VECTOR leftBonePos = MV1GetFramePosition(m_modelHandle, MV1SearchFrame(
		m_modelHandle, left_wing_bone_name));//左の羽の位置を取得
	VECTOR rightBonePos = MV1GetFramePosition(m_modelHandle, MV1SearchFrame(
		m_modelHandle, right_wing_bone_name));//右の羽の位置を取得
	Vector3 leftWingPos = Vector3(leftBonePos.x, leftBonePos.y, leftBonePos.z);
	Vector3 rightWingPos = Vector3(rightBonePos.x, rightBonePos.y, rightBonePos.z);

	//羽よりもすこし左右にずらす
	leftWingPos.x -= sea_splash_offset_x;
	rightWingPos.x += sea_splash_offset_x;
	//羽よりもすこし後ろにずらす
	leftWingPos.z -= sea_splash_offset_z;
	rightWingPos.z -= sea_splash_offset_z;

	//この二つの位置に水しぶきのエフェクトを出す
	UpdateWingSplash(m_leftWingEffectH, leftWingPos,ResourceLoader::EffectID::LeftWingSplash);
	UpdateWingSplash(m_rightWingEffectH, rightWingPos, ResourceLoader::EffectID::RightWingSplash);

	//無敵時間の更新
	if(m_invincibleFrame > 0)
	{
		m_invincibleFrame--;
	}

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
	float distZ = std::abs((playerPos - cameraPos).z);

	// 求めたdistZを使用してプレイヤーが移動できる範囲を
	// ワールド座標で算出する
	Vector2 frustumHalf = pCamera->GetFrustumHalfSize(clamp_frustum_distz);

	// プレイヤーの位置をそれぞれ求めた範囲でクランプする
	//-screenWToWorld～screenWToWorldがクランプ範囲
	m_pos.x = std::clamp(m_pos.x, -frustumHalf.x, frustumHalf.x);
	// 海面と、視錐台の下限を比べて制限が厳しい方を実際の下限として使用する
	m_pos.y = std::clamp(
		m_pos.y,
		std::max(-frustumHalf.y, Game::sea_player_margin),
		frustumHalf.y);
}

void Player::Somersault(InputManager& input)
{
	bool isSomersoult = false;
	if (std::dynamic_pointer_cast<SomersaultState>(m_pSpecialState))
	{
		isSomersoult = true;
	}

	// 宙返りボタンが押されていたらステートをそれぞれ切り替える
	if (input.IsTriggered(InputEvent::somersault) &&
		m_gauge > somersoult_use_gauge && 
		!isSomersoult)
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

void Player::UpdateWingSplash(int& splashHandle, const Vector3& wingPos, ResourceLoader::EffectID effectID)
{
	std::shared_ptr<EffectManager> pEffectManager = m_pEffectManager.lock();

	//羽と海面の距離を計算
	float distWingToSea = wingPos.y - sea_height;
	//海面付近にいるかをチェック
	if(distWingToSea < sea_splash_height_threshold)
	{
		//既に再生されていれば処理を飛ばす
		if (splashHandle == -1 ||
			!pEffectManager->IsPlaying(splashHandle))
		{
			//海面付近にいるので水しぶきのエフェクトを出す
			splashHandle = pEffectManager->Play(effectID, wingPos);
		}
		//位置更新
		//水しぶきの高さのみ海に合わせる
		pEffectManager->SetPos(splashHandle, Vector3(wingPos.x, sea_height, wingPos.z));

		//エフェクトの向きを機体のY軸回転に合わせる。
		//水面から上がる表現なのでXとZは反映しない。
		//プレイヤーモデルは逆向きに作られているのでBoostと同じく+DX_PI_Fで補正
		pEffectManager->SetRotation(splashHandle, Vector3(0.0f, GetRotationY() + DX_PI_F, 0.0f));
		//海面との距離を0～1の比率にしてエフェクトに渡す（海面で0、しきい値で1）
		float heightRate = std::clamp(distWingToSea / sea_splash_height_threshold, 0.0f, 1.0f);
		pEffectManager->SetDynamicInput(splashHandle, 0, heightRate);
		//海面との距離を元にアルファ値を計算してエフェクトに渡す
		int alpha = static_cast<int>(255.0f * std::clamp((1.0f - heightRate) * sea_splash_fade_scale, 0.0f, 1.0f));
		pEffectManager->SetColor(splashHandle, 255, 255, 255, alpha);
	}
	else
	{
		//海面付近にいないので水しぶきのエフェクトを止める
		if (splashHandle != -1)
		{
			pEffectManager->Stop(splashHandle);
			splashHandle = -1;
		}
	}
}

bool Player::IsNearSea() const
{
	//機体の中心と海面の距離
	float distance = m_pos.y - sea_height;
	return distance < sea_roll_limit_height;
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
	if (DebugManager::GetInstance().IsDebugDrawEnabled())
	{
		DrawFormatString(0, 300, 0xffffff, L"playerPosX:%f,Y : %f,Z:%f", m_pos.x, m_pos.y, m_pos.z);
		DrawFormatString(0, 250, 0xffffff, L"Gauge : %f", m_gauge);
		DrawFormatString(1080, 20, 0xffffff, L"Health : %d", m_health);
		DrawFormatString(0, 365, 0xffffff, L"IsFocus : %d", IsFocus());

		// 当たり判定の球を描画
		m_pHitCollider->GetSphere()->Draw(0xff0000);
		if (m_pCounterCollider->IsCollisionActive())
		{
			m_pCounterCollider->GetSphere()->Draw(0x0000ff);
		}
	}
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

	LightingManager::GetInstance().ApplyShader(false,0.0f);
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
	pSoundManager->Play(SoundManager::SoundType::PlayerDamage, false, false);

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

void Player::LerpToAngleZ(float targetAngle, float t)
{
	// targetAngleに向けてrotationZをLerpする
	m_rotationZ = m_rotationZ * (1 - t) + targetAngle * t;
	// Rotationを適用する
	UpdateRotation();
}

void Player::AddRotationZ(float delta)
{
	//rotationZに直接角度加算
	m_rotationZ += delta;
	if (m_rotationZ > DX_PI_F) m_rotationZ -= DX_TWO_PI_F;
	if (m_rotationZ < -DX_PI_F) m_rotationZ += DX_TWO_PI_F;
	//Rotationを適用
	UpdateRotation();
}

float Player::GetMaxSpecialGauge() const
{
	return max_gauge;
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
	//操作できない間に撃たれ続けて死なないよう、被弾しない状態にする
	m_isDisabled = true;

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
			std::static_pointer_cast<Player>(shared_from_this()), m_pBulletManager, m_pSoundManager,m_pTargetManager);
	ChangeShootState(newShootState);
}

void Player::ChangeAllStateToNormal()
{
	//被弾する状態に戻す
	m_isDisabled = false;

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
			std::static_pointer_cast<Player>(shared_from_this()), m_pBulletManager, m_pSoundManager,m_pTargetManager);
	ChangeShootState(newShootState);
}

bool Player::IsRolling() const
{
	auto rotationState = std::dynamic_pointer_cast<DefaultRotationState>(m_pRotationState);
	//DefaultRotationStateから回転中か取得
	if (rotationState)
	{
		return rotationState->IsRolling();
	}
	else return false;
}

void Player::OnInvincibleStart(int invinsibleFrame)
{
	// 無敵時間を開始する
	m_invincibleFrame = invinsibleFrame;
}

void Player::UpdateRotation()
{
	// XとYの回転角からQuaternionを生成
	Quaternion rotX = Quaternion(Vector3(1.0f, 0.0f, 0.0f), m_rotationX);
	Quaternion rotY = Quaternion(Vector3(0.0f, 1.0f, 0.0f), m_rotationY);
	Quaternion rotZ = Quaternion(Vector3(0.0f, 0.0f, 1.0f), m_rotationZ);
	// 掛け合わせたものをrotationとする
	m_rotation = rotX * rotY * rotZ;
}

std::shared_ptr<SphereShape> Player::GetHitSphere() const
{
	return m_pHitCollider->GetSphere();
}

std::shared_ptr<SphereShape> Player::GetCounterSphere() const
{
	return m_pCounterCollider->GetSphere();
}

ICollider& Player::GetHitCollider()
{
	//コライダーを取得
	//間接参照
	return *m_pHitCollider;
}

ICollider& Player::GetCounterCollider()
{
	//コライダーを取得
	//間接参照
	return *m_pCounterCollider;
}
