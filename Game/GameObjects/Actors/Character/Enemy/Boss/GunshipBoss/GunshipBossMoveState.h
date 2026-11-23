#pragma once
#include "Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/IGunshipBossState.h"
#include "Utility/Vector3.h"

class GunshipBoss;
class ModelAnimator;
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
	void UpdateMoving();
	//移動地点を決める
	void StartMove();
	//止まっている間の更新処理
	void UpdateStopping();

private:
	//移動開始地点
	Vector3 m_startPos;
	//移動終了地点
	Vector3 m_endPos;
	//時間
	int m_frame = 0;
	//現在動いているか止まっているか
	bool m_isMoving = true;

	//移動した回数
	int m_moveCount = 0;

	//アニメーター
	ModelAnimator& m_animator;
};
