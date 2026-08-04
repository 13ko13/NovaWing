#pragma once
#include "IFloatingEnemyState.h"

class FloatingEnemy;
class LeaveState : public IFloatingEnemyState
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

	//進行方向を決めたかどうか
	bool m_isDecideDir = false;

	//逃げていく方向
	enum class LeaveDirection
	{
		Right, // 右
		Left,  // 左

		Max,//最大
	};

	//逃げていく方向
	LeaveDirection m_leaveDir;
};

