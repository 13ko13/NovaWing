#pragma once
#include "IEnemyState.h"

class FloatingEnemy;
class LeaveState : public IEnemyState
{
public:
	LeaveState(std::weak_ptr<FloatingEnemy> pEnemy);
	~LeaveState();

	void Enter() override;
	void Update() override;
	void Exit() override;

private:
	//死亡するまでの時間計測
	int m_frame = 0;
};

