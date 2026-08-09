#include <EffekseerForDXLib.h>

#include "SummonState.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/EnemyFactory.h"
#include "BossIdleState.h"
#include "Manager/ResourceLoader.h"
#include "BossEnemy.h"

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

	//生成した敵に応じて再生するエフェクトを変える
	int handle = -1;
	switch (static_cast<EnemyFactory::EnemyType>(rand))
	{
	case EnemyFactory::EnemyType::FloatingEnemy:
		//ハンドルを取得
		handle = ResourceLoader::GetInstance().GetEffect(ResourceLoader::EffectID::SummonFloating);

		//エフェクトを再生
		m_floatingEffPlayH = PlayEffekseer3DEffect(handle);
		//位置を設定
		SetPosPlayingEffekseer3DEffect(
			m_floatingEffPlayH,
			m_summonPos.m_x, 
			m_summonPos.m_y,
			m_summonPos.m_z
		);
		break;
	case EnemyFactory::EnemyType::WormEnemy:
		//ハンドルを取得
		handle = ResourceLoader::GetInstance().GetEffect(ResourceLoader::EffectID::SummonWorm);

		//エフェクトを再生
		m_wormEffPlayH = PlayEffekseer3DEffect(handle);
		//位置を設定
		SetPosPlayingEffekseer3DEffect(
			m_wormEffPlayH,
			m_summonPos.m_x,
			m_summonPos.m_y,
			m_summonPos.m_z
		);
		break;
	}

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
	//ボスが死んでいるなら処理を行わない
	if (m_pBoss.lock() == nullptr) return;

	//召喚は終了しているのでidleに戻る
	ChangeState(
		std::make_shared<BossIdleState>(m_pBoss, m_pBoss.lock()->GetPlayer())
	);
}

void SummonState::Exit()
{
}
