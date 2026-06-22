#pragma once
#include "IMovementState.h"
class MovingState : public IMovementState
{
public:
	MovingState(const std::weak_ptr<Player> pPlayer);
	~MovingState();

	void Enter() override;
	void Update() override;
	void Exit() override;

private:

};

