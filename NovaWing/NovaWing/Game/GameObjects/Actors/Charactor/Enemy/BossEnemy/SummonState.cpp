#include "SummonState.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/EnemyFactory.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/BossEnemy/BossIdleState.h"

SummonState::SummonState(
	std::weak_ptr<BossEnemy> pBoss,
	const Position3& summonPos,
	std::weak_ptr<EnemyFactory> pEnemyFactory) :
	IBossEnemyState(pBoss),
	m_summonPos(summonPos),
	m_pEnemyFactory(pEnemyFactory)
{
}

SummonState::~SummonState()
{
}

void SummonState::Enter()
{
	//浮遊敵とワームエネミーをランダムで召喚
	//GetRandが0を含むので-1
	int rand = GetRand(
		static_cast<int>(EnemyFactory::EnemyType::Max) - 1
	);

	//敵生産工場に生産させる
	if (m_pEnemyFactory.lock() != nullptr)
	{
		m_pEnemyFactory.lock()->Create(
		m_summonPos,
		static_cast<EnemyFactory::EnemyType>(rand)
		);
	}
}

void SummonState::Update()
{
	//召喚は終了しているのでidleに戻る
	ChangeState(
		std::make_shared<BossIdleState>(m_pBoss)
	);
}

void SummonState::Exit()
{
}
