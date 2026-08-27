#define NOMINMAX

#include "TargetManager.h"
#include "Charactor/Player/Player.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/EnemyBase.h"

namespace
{
	//フォーカス可能な距離
	constexpr float focus_range = 700.0f;

	//手前のレティクルが見るZ値
	constexpr float front_reticle_distance = 1400.0f;
	//奥のレティクルが見るZ値
	constexpr float reticle_distance = 1800.0f;
}

TargetManager::TargetManager(std::weak_ptr<Player> pPlayer):
	m_pPlayer(pPlayer)
{
}

TargetManager::~TargetManager()
{
}

void TargetManager::Update()
{
	//プレイヤーをshared_ptrに変換
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();

	//敵をすべてshared_ptrに変換
	std::vector<std::shared_ptr<EnemyBase>> pEnemies;
	for (std::weak_ptr<EnemyBase>& weakEnemy : m_pEnemies)
	{
		std::shared_ptr<EnemyBase> pEnemy = weakEnemy.lock();
		//nullじゃなければ配列に保持
		if (pEnemy == nullptr) continue;

		//配列に保持
		pEnemies.push_back(pEnemy);
	}

	//プレイヤーより前方にいる敵をフォーカス候補とする
	for (std::shared_ptr<EnemyBase> pEnemy : pEnemies) 
	{
		//敵の位置
		Vector3 enemyPos = pEnemy->GetPos();
		//プレイヤーの位置
		Vector3 playerPos = m_pPlayer.lock()->GetPos();
		if (enemyPos.m_z < playerPos.m_z) continue;


	}


	//死んでたら配列から削除
	m_pEnemies.erase(
	std::remove_if(m_pEnemies.begin(), m_pEnemies.end(),
		[](const std::weak_ptr<EnemyBase>& pEnemy)
		{
			return pEnemy.lock() == nullptr;
		}),
	m_pEnemies.end()
	);
}

void TargetManager::Register(std::shared_ptr<EnemyBase> pEnemy)
{
	//配列に追加
	m_pEnemies.push_back(pEnemy);
}
