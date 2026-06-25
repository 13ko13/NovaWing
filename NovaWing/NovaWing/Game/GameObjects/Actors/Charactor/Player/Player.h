#pragma once
#include <memory>

#include "../Charactor.h"

class InputManager;
class BulletManager;
class CameraBase;
class IMovementState;
class IRotationState;
class IShootState;
class Player : public Charactor
{
public:
	Player(std::shared_ptr<BulletManager> bulletManager,
		ResourceLoader::ModelID modelID);
	~Player();

	void OnInit() override;
	void Update() override;

	void Draw() override;
	
	void TakeDamage(int damage) override;

	//プレイヤーのモデルが逆向きなので
	//初期回転を常に保存しておく
	Quaternion GetInitRotation() const { return m_initRotation; }

	//AngleXまでLerpする
	void LerpToAngleX(float targetAngle,float t);
	//AngleYまでLerpする
	void LerpToAngleY(float targetAngle, float t);

	//GameSceneからカメラをセットさせる
	void SetCamera(std::shared_ptr<CameraBase> pCamera) { m_pCamera = pCamera; }

private:
	//回転の更新
	void UpdateRotation();
	//シェーダを適用したプレイヤーの描画
	void DrawPlayer();
	//行列情報を定数バッファに設定する
	void SetCbuffMatrixData();
	//移動範囲を制限する
	void ClampPosition();
	
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
	int m_boostGauge = 100;//ブーストゲージ
	float m_boostSpeed = 0;//ブースト時のスピード
	float m_brakeSpeed = 0;//ブレーキ時のスピード

	//外部クラス参照
	//借りてくるだけなのでweak_ptrにする
	std::weak_ptr<BulletManager> m_pBulletManager;//弾の管理
	std::weak_ptr<CameraBase> m_pCamera;//カメラ

	//プレイヤーの向きが逆向きなので
	//プレイヤーの初期回転を保存する
	Quaternion m_initRotation;

	//上下回転角
	float m_rotationX = 0.0f;
	//左右回転角
	float m_rotationY = 0.0f;

	//シェーダに渡す情報
	struct MatrixBuffer
	{
		MATRIX world;//ワールド行列
		MATRIX view;//ビュー行列
		MATRIX proj;//プロジェクション行列
	};
	int m_cbufferMatrix = -1;
	MatrixBuffer* m_pCbufferMatrixData = nullptr;

	struct CameraBuffer
	{
		Vector3 cameraPos;//カメラの位置
		float padding;
	};
	int m_cbufferCamera = -1;
	CameraBuffer* m_pCbufferCameraData = nullptr;

	//移動系ステート
	std::shared_ptr<IMovementState> m_pMovementState;
	//回転系ステート
	std::shared_ptr<IRotationState> m_pRotationState;
	//弾撃ち系ステート
	std::shared_ptr<IShootState> m_pShootState;
};