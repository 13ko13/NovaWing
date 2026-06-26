#pragma once
#include "IShootState.h"

class Player;
class BulletManager;
class NormalShootState : public IShootState
{
public:
	NormalShootState(const std::weak_ptr<Player> pPlayer,
		std::weak_ptr<BulletManager> pBulletManager);
	~NormalShootState();

	void Exit() override;
	void Update() override;
	void Enter() override;

private:
};

