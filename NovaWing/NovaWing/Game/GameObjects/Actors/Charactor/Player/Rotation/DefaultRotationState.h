#pragma once
#include "IRotationState.h"
class DefaultRotationState : public IRotationState
{
public:
	DefaultRotationState(const std::weak_ptr<Player> pPlayer);
	~DefaultRotationState();

	void Enter()override;//ステートに入った時
	void Update() override;//更新処理
	void Exit() override;//ステートから出たとき

private:
	//ローリングボタンを押してからのフレーム
	int m_pushRightRollFrame = 0;
	int m_pushLeftRollFrame = 0;
	//ローリングを開始しているか
	bool m_isStartRolling = false;
};