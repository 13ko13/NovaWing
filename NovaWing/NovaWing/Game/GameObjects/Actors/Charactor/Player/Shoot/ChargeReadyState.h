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
	//チャージが完了して
	//次またボタンを押されるまでの時間
	int m_waitFrame = 0;
};