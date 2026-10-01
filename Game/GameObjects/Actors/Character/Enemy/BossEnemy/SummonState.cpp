#include <EffekseerForDXLib.h>

#include "SummonState.h"
#include "Game/GameObjects/Actors/Character/Enemy/EnemyFactory.h"
#include "BossIdleState.h"
#include "Manager/ResourceLoader.h"
#include "Manager/SoundManager.h"
#include "BossEnemy.h"

namespace
{
	//召喚時のエフェクトの再生位置のオフセット
	const Vector3 summon_effect_offset = Vector3(300.0f, 0.0f, 0.0f);
}

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
	//召喚音を鳴らす
	m_pBoss.lock()->GetSoundManager().lock()->Play(SoundManager::SoundType::BossSummon);

	//浮遊敵とワームエネミーをランダムで召喚
	//GetRandが0を含むので-1
	int rand = GetRand(
		static_cast<int>(EnemyFactory::EnemyType::Max) - 1
	);

	//左右対称にして2か所から召喚
	Vector3 leftSummonPos = m_summonPos - summon_effect_offset;
	Vector3 rightSummonPos = m_summonPos + summon_effect_offset;

	//生成した敵に応じて再生するエフェクトを変える
	int handle = -1;
	switch (static_cast<EnemyFactory::EnemyType>(rand))
	{
	case EnemyFactory::EnemyType::FloatingEnemy:
		//ハンドルを取得
		handle = ResourceLoader::GetInstance().GetEffect(ResourceLoader::EffectID::SummonFloating);

		//エフェクトを再生
		m_floatingEffRightPlayH = PlayEffekseer3DEffect(handle);
		m_floatingEffLeftPlayH = PlayEffekseer3DEffect(handle);
		//位置を設定
		SetPosPlayingEffekseer3DEffect(
			m_floatingEffRightPlayH,
			leftSummonPos.x, 
			leftSummonPos.y,
			leftSummonPos.z
		);
		SetPosPlayingEffekseer3DEffect(
			m_floatingEffLeftPlayH,
			rightSummonPos.x, 
			rightSummonPos.y,
			rightSummonPos.z
		);
		break;
	case EnemyFactory::EnemyType::WormEnemy:
		//ハンドルを取得
		handle = ResourceLoader::GetInstance().GetEffect(ResourceLoader::EffectID::SummonWorm);

		//エフェクトを再生
		m_wormEffLeftPlayH = PlayEffekseer3DEffect(handle);
		m_wormEffRightPlayH = PlayEffekseer3DEffect(handle);
		//位置を設定
		SetPosPlayingEffekseer3DEffect(
			m_wormEffLeftPlayH,
			leftSummonPos.x,
			leftSummonPos.y,
			leftSummonPos.z
		);
		SetPosPlayingEffekseer3DEffect(
			m_wormEffRightPlayH,
			rightSummonPos.x,
			rightSummonPos.y,
			rightSummonPos.z
		);
		break;
	}

	//敵生産工場に生産させる
	if (m_pEnemyFactory.lock() != nullptr)
	{
		//召喚
		m_pEnemyFactory.lock()->Create(
			leftSummonPos,
			static_cast<EnemyFactory::EnemyType>(rand)
		);
		m_pEnemyFactory.lock()->Create(
			rightSummonPos,
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
