#pragma once
#include "IShootState.h"

class Player;
class BulletManager;
class ChargeReadyState : public IShootState
{
public:
	ChargeReadyState(const std::weak_ptr<Player> pPlayer,
		std::weak_ptr<BulletManager> pBulletManager);
	~ChargeReadyState();

	void Exit() override;
	void Update() override;
	void Enter() override;

private:
	//チャージ中のプレイエフェクト
	int m_chargingPlayEffectH = -1;
};