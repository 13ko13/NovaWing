#include <DxLib.h>
#include <cassert>

#include "SpiderBossIdleState.h"
#include "SpiderBossSummonState.h"
#include "SpiderBoss.h"
#include "Game/GameObjects/Actors/Character/Player/Player.h"
#include "SpiderBossBeamState.h"

namespace
{
	//攻撃をどのぐらいの間隔で行うか
	constexpr int attack_interval = 60 * 3;
	//召喚位置オフセット
	const Vector3 summon_pos_offset = Vector3(0.0f, 600.0f, -1550.0f);
}

SpiderBossIdleState::SpiderBossIdleState(
	std::weak_ptr<SpiderBoss> pBoss,
	std::weak_ptr<Player> pPlayer) :
	ISpiderBossState(pBoss),
	m_pPlayer(pPlayer)
{
}

SpiderBossIdleState::~SpiderBossIdleState()
{
}

void SpiderBossIdleState::Enter()
{
	//念のため攻撃フレームをリセット
	m_attackFrame = 0;
}

void SpiderBossIdleState::Update()
{
	//攻撃フレーム更新
	m_attackFrame++;
	//attack_intervalごとに攻撃ステートへチェンジする
	if (m_attackFrame % attack_interval == 0)
	{
		//攻撃の種類の中からランダムで行う
		//GetRandが0を含むので-1
		int rand = GetRand(
			static_cast<int>(AttackType::Max) - 1
		);

		//ボスが死んでいるなら処理を行わない
		if (m_pBoss.lock() == nullptr) return;

		//攻撃の種類によってそれぞれのステートに遷移
		switch (static_cast<AttackType>(rand))
		{
		case AttackType::Summon://雑魚召喚
		{
			//召喚位置を決める
			Vector3 summonPos;
			//ボスの位置から少し離れた位置に召喚する
			summonPos = m_pBoss.lock()->GetPos();
			summonPos += summon_pos_offset;
			
			ChangeState(std::make_shared<SpiderBossSummonState>(
				m_pBoss,
				summonPos,
				m_pBoss.lock()->GetEnemyFactory()
			));
		}
		break;

		case AttackType::Beam://ビーム

			//ビームステートに変更
			ChangeState(
				std::make_shared<SpiderBossBeamState>(
					m_pBoss, m_pPlayer)
				);

			break;

		default:
			//それ以外の値ならおかしいのでクラッシュさせる
			assert(-1 && L"行動のランダム値が不適切な値です");
		}
	}
}

void SpiderBossIdleState::Exit()
{
}
