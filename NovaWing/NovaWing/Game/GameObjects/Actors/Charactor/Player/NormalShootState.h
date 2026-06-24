#pragma once
#include "IShootState.h"
class NormalShootState : public IShootState
{
public:
	NormalShootState(const std::weak_ptr<Player> pPlayer);
	~NormalShootState();

	void Exit() override;
	void Update() override;
	void Enter() override;

private:
};

