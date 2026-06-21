#pragma once
#include <memory>

#include "../Charactor.h"

class InputManager;
class BulletManager;
class IPlayerState;
class CameraBase;
class Player : public Charactor
{
public:
	Player(std::shared_ptr<BulletManager> bulletManager,
		ResourceLoader::ModelID modelID);
	~Player();

	void OnInit() override;
	void Update() override;
	void Draw() override;
	void ApplyMatrix();
	void TakeDamage(int damage) override;

	//プレイヤーのモデルが逆向きなので
	//初期回転を常に保存しておく
	Quaternion GetInitRotation() const { return m_initRotation; }

	//上下回転
	void RotateX(float angle);
	//左右回転
	void RotateY(float angle);

	//Lerpを適用したRotation
	void LerpRotation(float t);

	//GameSceneからカメラをセットさせる
	void SetCamera(std::shared_ptr<CameraBase> pCamera) { m_pCamera = pCamera; }

private:
	//ステートの変更
	void ChangeState(std::shared_ptr<IPlayerState> pNextState);
	//回転の更新
	void UpdateRotation();
	//シェーダを適用したプレイヤーの描画
	void DrawPlayer();

	void SetCbuffMatrixData();

private:
	//ブースト関連
	int m_boostGauge = 100;//ブーストゲージ
	float m_boostSpeed = 0;//ブースト時のスピード
	float m_brakeSpeed = 0;//ブレーキ時のスピード

	//外部クラス参照
	//借りてくるだけなのでweak_ptrにする
	std::weak_ptr<BulletManager> m_pBulletManager;//弾の管理
	std::weak_ptr<CameraBase> m_pCamera;//カメラ

	//現在のステートを持つ
	std::shared_ptr<IPlayerState> m_pCurrentState;

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
};