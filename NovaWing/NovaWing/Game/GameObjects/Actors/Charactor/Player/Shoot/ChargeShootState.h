#pragma once
#include "IShootState.h"

class Player;
class BulletManager;
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
};

