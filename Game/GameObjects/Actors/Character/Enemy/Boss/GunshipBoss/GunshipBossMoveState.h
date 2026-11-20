#pragma once
#include "Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/IGunshipBossState.h"
#include "Utility/Vector3.h"

class GunshipBoss;
class GunshipBossMoveState : public IGunshipBossState
{
public:
	GunshipBossMoveState(std::weak_ptr<GunshipBoss> pBoss);
	virtual ~GunshipBossMoveState();

	void Enter() override;
	void Update() override;
	void Draw() override;
	void Exit() override;

private:
	//移動
	void Move();
	
private:
	//移動開始地点
	Vector3 m_startPos;
	//移動終了地点
	Vector3 m_endPos;
	//時間
	int m_frame = 0;

	//移動した回数
	int m_moveCount = 0;
};
