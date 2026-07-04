#pragma once
#include "IEnemyState.h"

class FloatingEnemy;
class ActiveState : public IEnemyState
{
public:
	ActiveState(std::weak_ptr<FloatingEnemy> pEnemy);
	~ActiveState();

	void Enter() override;
	void Update() override;
	void Exit() override;

private:
	//sin波用のフレーム
	int m_frame = 0;
};