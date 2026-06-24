#pragma once
#include "IShootState.h"
class ChargeShootState : public IShootState
{
public:
	ChargeShootState(const std::weak_ptr<Player> pPlayer);
	~ChargeShootState();

	void Exit() override;
	void Update() override;
	void Enter() override;

private:
	//チャージしている時間
	int m_chargeFrame = 0;
};

