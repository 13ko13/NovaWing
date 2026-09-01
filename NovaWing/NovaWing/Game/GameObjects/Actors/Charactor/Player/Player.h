#pragma once
#include <memory>
#include <set>

#include "../Charactor.h"
#include "Utility/Sphere.h"
#include "Manager/SoundManager.h"

class InputManager;
class BulletManager;
class CameraBase;
class IMovementState;
class IRotationState;
class IShootState;
class ISpecialActionState;
class TargetManager;
class EnemyBase;
class Player : public Charactor
{
public:
	Player(std::shared_ptr<BulletManager> bulletManager,
		ResourceLoader::ModelID modelID,
		std::weak_ptr<CameraBase> camera,
		std::weak_ptr<SoundManager> soundManager);
	~Player();

	void OnInit() override;
	void Update() override;

	void Draw() override;

	void TakeDamage(int damage) override;

	//プレイヤーのモデルが逆向きなので
	//初期回転を常に保存しておく
	Quaternion GetInitRotation() const { return m_initRotation; }

	//プレイヤーのモデルが逆向きなので、GetForward/GetBackはコード上と見た目が逆になる
	//見た目通りの前方向・後方向が欲しいときはこちらを使う
	Vector3 GetVisualForward() const { return GetBack(); }
	Vector3 GetVisualBack() const { return GetForward(); }

	//AngleXまでLerpする
	void LerpToAngleX(float targetAngle, float t);
	//AngleYまでLerpする
	void LerpToAngleY(float targetAngle, float t);

	//GameSceneからカメラをセットさせる
	void SetCamera(std::shared_ptr<CameraBase> pCamera) { m_pCamera = pCamera; }
	//ターゲットマネージャーをセットさせる
	void SetTargetManager(std::shared_ptr<TargetManager> pTargetManager) { m_pTargetManager = pTargetManager; }

	//ゲージ取得
	float GetSpecialGauge() const { return m_gauge; }
	//ゲージの最大値取得
	float GetMaxSpecialGauge() const;

	//ゲージ変化
	void ChangeGauge(float delta);

	//ゲージを使用するとき
	void StartUseGauge();
	//ゲージの使用をやめたとき
	void EndUseGauge();
	//ゲージを使用してるかを取得
	bool IsUseGauge() const;
	//当たり判定用の球を取得
	Sphere GetSphere() const { return m_collSphere; }

	//フォーカスターゲットを取得
	std::weak_ptr<EnemyBase> GetForcusTarget() const;
	//フォーカス中か
	bool IsFocus() const;
	//チャージ完了しているか
	bool IsChargeReady() const;

	//チャージ中エフェクトの再生ハンドルを取得
	int& GetChargingEffectHandle() { return m_chargingEffectH; }
	//チャージ中エフェクトの再生ハンドルを設定
	void SetChargingEffectHandle(int handle) { m_chargingEffectH = handle; }

	//今宙返り中かどうかを返す
	bool IsSomersault() const;

	//全てのステートを何もしないステートに変更する
	void ChangeAllStateToDisabled();
	//全てのステートを通常のステートに変更
	void ChangeAllStateToNormal();

	//被弾回数を取得
	int GetHitCount() const { return m_hitCount; }

	//倒した敵の数をインクリメントさせる
	void AddDefeatedEnemyCount() { m_defeatedEnemyCount++; }

	//倒した敵の数を取得
	int GetDefeatedEnemyCount() const { return m_defeatedEnemyCount; }

private:
	//回転の更新
	void UpdateRotation();
	//シェーダを適用したプレイヤーの描画
	void DrawPlayer();
	//移動範囲を制限する
	void ClampPosition();
	//宙返り処理
	void Somersault(InputManager& input);
	//ブースト入力処理
	void Boost(const InputManager& input);
	//ブレーキ入力処理
	void Brake(const InputManager& input);
	//移動系ステートを変更する
	void ChangeMovementState(std::shared_ptr<IMovementState>(newState));
	//発射系ステートを変更する
	void ChangeShootState(std::shared_ptr<IShootState>(newState));
	//回転系ステートを変更する
	void ChangeRotationState(std::shared_ptr<IRotationState>(newState));
	//特殊行動系ステートを変更する
	void ChangeSpecialState(std::shared_ptr<ISpecialActionState>(newState));

	//ステートの更新
	//テンプレート関数なのでヘッダに実装をかく
	template <typename T>
	void UpdateState(std::shared_ptr<T>& state)
	{
		//更新処理
		state->Update();
		//次のステートを取得してその中身がnullじゃなければ
		std::shared_ptr<T> next = state->GetNextState();

		if (next != nullptr)
		{
			state->Exit();
			state = next;
			state->Enter();
		}
	}

private:
	//ブースト関連
	float m_gauge = 0.0f;//ブーストゲージ
	float m_boostSpeed = 0;//ブースト時のスピード
	float m_brakeSpeed = 0;//ブレーキ時のスピード

	//ダメージエフェクトの経過時間
	int m_damageTime = 0;

	//ダメージシェーダ用の定数バッファ
	struct DamageBuffer
	{
		float redAmount;
		float paddng[3];//16バイトアライメント
	};
	int m_cbufferDamage = -1;
	DamageBuffer* m_pCBufferDamageData = nullptr;


	//外部クラス参照
	//借りてくるだけなのでweak_ptrにする
	std::weak_ptr<BulletManager> m_pBulletManager;//弾の管理
	std::weak_ptr<SoundManager> m_pSoundManager;//音の管理

	//プレイヤーの向きが逆向きなので
	//プレイヤーの初期回転を保存する
	Quaternion m_initRotation;

	//現在ゲージ消費中かどうか
	bool m_isUseGauge = false;

	//上下回転角
	float m_rotationX = 0.0f;
	//左右回転角
	float m_rotationY = 0.0f;

	//移動系ステート
	std::shared_ptr<IMovementState> m_pMovementState;
	//回転系ステート
	std::shared_ptr<IRotationState> m_pRotationState;
	//弾撃ち系ステート
	std::shared_ptr<IShootState> m_pShootState;
	//特殊行動系ステート
	std::shared_ptr<ISpecialActionState> m_pSpecialState;

	//当たり判定用の球
	Sphere m_collSphere;

	//ターゲットマネージャー
	std::weak_ptr<TargetManager> m_pTargetManager;

	//チャージ中エフェクト再生ハンドル
	int m_chargingEffectH = -1;

	//ダメージシェーダのハンドル
	int m_damageShaderPSH = -1;

	//ダメージエフェクトを出すか
	bool m_isDamageEffect = false;
	
	//被弾回数
	int m_hitCount = 0;

	//倒した敵の数
	int m_defeatedEnemyCount = 0;
};