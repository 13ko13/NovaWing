#pragma once
#include "IFloatingEnemyState.h"
class HideState : public IFloatingEnemyState
{
public:
	HideState(std::weak_ptr<FloatingEnemy> pEnemy);
	~HideState();

	void Enter() override;//ステートに入った時
	void Update() override;//更新処理
	void Exit() override;//ステートから出たとき
};

