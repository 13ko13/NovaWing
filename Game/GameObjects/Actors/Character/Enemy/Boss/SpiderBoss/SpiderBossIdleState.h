#pragma once
#include "ISpiderBossState.h"

class Player;
class SpiderBossIdleState : public ISpiderBossState
{
public:
	SpiderBossIdleState(std::weak_ptr<SpiderBoss> pBoss,
		std::weak_ptr<Player> pPlayer);
	~SpiderBossIdleState();

	void Enter() override;
	void Update() override;
	void Exit() override;

private:
	//攻撃フレーム更新
	int m_attackFrame = 0;

	//攻撃の種類
	enum class AttackType
	{
		Summon,//召喚
		Beam,//ビーム

		Max,//数
	};

	//プレイヤーの弱参照
	std::weak_ptr<Player> m_pPlayer;
};

