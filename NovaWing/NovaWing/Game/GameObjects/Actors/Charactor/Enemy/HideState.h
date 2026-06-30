#pragma once
#include "IEnemyState.h"
class HideState : public IEnemyState
{
public:
	HideState(std::weak_ptr<FloatingEnemy> pEnemy);
	~HideState();

	void Enter() override;//ステートに入った時
	void Update() override;//更新処理
	void Exit() override;//ステートから出たとき
};

