#pragma once
#include <memory>

#include "../Charactor.h"

class InputManager;
class BulletManager;
class IPlayerState;
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

private:
	void ChangeState(std::shared_ptr<IPlayerState> pNextState);

private:
	//ブースト関連
	int m_boostGauge = 100;//ブーストゲージ
	float m_boostSpeed = 0;//ブースト時のスピード
	float m_brakeSpeed = 0;//ブレーキ時のスピード

	//外部クラス参照
	//借りてくるだけなのでweak_ptrにする
	std::weak_ptr<BulletManager> m_pBulletManager;

	//現在のステートを持つ
	std::shared_ptr<IPlayerState> m_pCurrentState;

	//プレイヤーの向きが逆向きなので
	//プレイヤーの初期回転を保存する
	Quaternion m_initRotation;
};

