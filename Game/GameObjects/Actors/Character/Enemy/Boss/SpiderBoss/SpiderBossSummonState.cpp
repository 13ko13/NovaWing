#include "SpiderBossSummonState.h"
#include "Game/GameObjects/Actors/Character/Enemy/EnemyFactory.h"
#include "SpiderBossIdleState.h"
#include "Manager/ResourceLoader.h"
#include "Manager/SoundManager.h"
#include "Manager/EffectManager.h"
#include "SpiderBoss.h"

namespace
{
	//召喚時のエフェクトの再生位置のオフセット
	const Vector3 summon_effect_offset = Vector3(300.0f, 0.0f, 0.0f);

	//召喚してよい敵の種類を並べる
	constexpr EnemyFactory::EnemyType summon_enemy_types[] =
	{
		EnemyFactory::EnemyType::FloatingEnemy,
		EnemyFactory::EnemyType::WormEnemy
	};
}

SpiderBossSummonState::SpiderBossSummonState(
	std::weak_ptr<SpiderBoss> pBoss,
	const Position3& summonPos,
	std::weak_ptr<EnemyFactory> pEnemyFactory) :
	ISpiderBossState(pBoss),
	m_summonPos(summonPos),
	m_pEnemyFactory(pEnemyFactory)
{
}

SpiderBossSummonState::~SpiderBossSummonState()
{
	
}

void SpiderBossSummonState::Enter()
{
	//召喚音を鳴らす
	m_pBoss.lock()->GetSoundManager().lock()->Play(SoundManager::SoundType::BossSummon);

	//浮遊敵とワームエネミーをランダムで召喚
	const int enemyTypeCount = std::size(summon_enemy_types);
	//GetRandは0〜引数(引数も含む)を返すので、種類の数-1を渡す
	int rand = GetRand(enemyTypeCount - 1);
	EnemyFactory::EnemyType enemyType = summon_enemy_types[rand];

	//左右対称にして2か所から召喚
	Vector3 leftSummonPos = m_summonPos - summon_effect_offset;
	Vector3 rightSummonPos = m_summonPos + summon_effect_offset;

	//生成した敵に応じて再生するエフェクトを変える
	std::shared_ptr<EffectManager> pEffectManager = m_pBoss.lock()->GetEffectManager().lock();
	switch (static_cast<EnemyFactory::EnemyType>(enemyType))
	{
	case EnemyFactory::EnemyType::FloatingEnemy:
		//左右2か所にエフェクトを再生
		pEffectManager->PlayOneShot(ResourceLoader::EffectID::SummonFloating, leftSummonPos);
		pEffectManager->PlayOneShot(ResourceLoader::EffectID::SummonFloating, rightSummonPos);
		break;
	case EnemyFactory::EnemyType::WormEnemy:
		//左右2か所にエフェクトを再生
		pEffectManager->PlayOneShot(ResourceLoader::EffectID::SummonWorm, leftSummonPos);
		pEffectManager->PlayOneShot(ResourceLoader::EffectID::SummonWorm, rightSummonPos);
		break;
	}

	//敵生産工場に生産させる
	if (m_pEnemyFactory.lock() != nullptr)
	{
		//召喚
		m_pEnemyFactory.lock()->Create(
			leftSummonPos,
			enemyType
		);
		m_pEnemyFactory.lock()->Create(
			rightSummonPos,
			enemyType
		);
	}
}

void SpiderBossSummonState::Update()
{
	//ボスが死んでいるなら処理を行わない
	if (m_pBoss.lock() == nullptr) return;
	//召喚は終了しているのでidleに戻る
	ChangeState(
		std::make_shared<SpiderBossIdleState>(m_pBoss, m_pBoss.lock()->GetPlayer())
	);
}

void SpiderBossSummonState::Exit()
{
}
