#pragma once
#include "IShootState.h"
#include "Utility/Vector3.h"

class Player;
class BulletManager;
class SoundManager;
class ChargeReadyState : public IShootState
{
public:
	ChargeReadyState(const std::weak_ptr<Player> pPlayer,
		std::weak_ptr<BulletManager> pBulletManager,
		std::weak_ptr<SoundManager> pSoundManager,
		std::weak_ptr<TargetManager> pTargetManager);
	~ChargeReadyState();

	void Exit() override;
	void Update() override;
	void Enter() override;

private:
	//チャージ中のプレイエフェクト
	int m_chargingPlayEffectH = -1;

	//ボタンを離してからのフレームを計測する
	int m_notPressdFrame = 0;

	//エフェクトの大きさを変更するためにメンバ変数で保持
	Vector3 m_effectScale;

	//エフェクトのサイズを小さくするかのフラグ
	bool m_canShrink = false;
};