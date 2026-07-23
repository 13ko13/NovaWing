#pragma once
#include "IShootState.h"
#include "Utility/Vector3.h"

class Player;
class BulletManager;
class TargetManager;
class ChargeShootState : public IShootState
{
public:
	ChargeShootState(const std::weak_ptr<Player> pPlayer,
		std::weak_ptr<BulletManager> pBulletManager);
	~ChargeShootState();

	void Exit() override;
	void Update() override;
	void Enter() override;

private:
	//チャージしている時間
	int m_chargeFrame = 0;
	//チャージ中のプレイエフェクト
	int m_chargingPlayEffectH = -1;
	//エフェクトの大きさを変更するため保持しておく
	Vector3 m_effectScale;
};

