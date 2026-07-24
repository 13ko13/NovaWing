#define NOMINMAX

#include "TargetManager.h"
#include "Charactor/Player/Player.h"
#include "Charactor/Enemy/FloatingEnemy/FloatingEnemy.h"
#include "Charactor/Enemy/WormEnemy/WormEnemy.h"

namespace
{
	//レティクルの距離
	constexpr float reticle_distance = 1800.0f;
	//フォーカス可能な距離
	constexpr float focus_range = 1000.0f;

	//手前のレティクルが見るZ値
	constexpr float front_reticle_distance = 1400.0f;
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
	std::vector<std::shared_ptr<FloatingEnemy>> pFloatingEnemies;
	std::vector<std::shared_ptr<WormEnemy>> pWormEnemies;
	for (std::weak_ptr<FloatingEnemy>& weakEnemy : m_pFloatingEnemies)//浮遊敵
	{
		pFloatingEnemies.push_back(weakEnemy.lock());
	}
	for (std::weak_ptr<WormEnemy>& weakEnemy : m_pWormEnemies)
	{
		pWormEnemies.push_back(weakEnemy.lock());
	}

	//手前のレティクルの位置を計算
	m_frontReticlePos = pPlayer->GetPos() +
		(-pPlayer->GetForward() * front_reticle_distance);

	//レティクルの位置を計算する
	//レティクルはプレイヤーが見ている方向に応じて移動するようにするので
	//プレイヤーの前方向を取得して、そこに一定距離をかけた位置をレティクルの位置とする
	m_reticlePos = pPlayer->GetPos() + (-pPlayer->GetForward() * reticle_distance);

	//レティクルに最も近い敵を探すために
	//今までで一番近い距離を記録してくれる変数を用意する
	float minDistance = FLT_MAX;//floatの最大値を入れておく
	//今までで一番近い敵のオブジェクト
	std::shared_ptr<GameObject> pNearestTarget;

	//全ての敵をループして、レティクルに最も近い敵を探す
	for (std::shared_ptr<FloatingEnemy>& pFloatingEnemy : pFloatingEnemies)
	{
		//浮遊敵の位置を取得
		Vector3 enemyPos = pFloatingEnemy->GetPos();
		//レティクルから敵までの距離を計算
		Vector3 toEnemy = enemyPos - m_reticlePos;
		float distance = toEnemy.Length();

		//その距離が今までで一番近い距離より近ければ
		//今一番近い敵として保存する
		if (distance < minDistance)
		{
			minDistance = distance;
			pNearestTarget = pFloatingEnemy;
		}
	}
	for (std::shared_ptr<WormEnemy>& pWormEnemy : pWormEnemies)
	{
		//ワーム敵の位置を取得
		Vector3 enemyPos = pWormEnemy->GetPos();
		//レティクルから敵までの距離を計算
		Vector3 toEnemy = enemyPos - m_reticlePos;
		float distance = toEnemy.Length();

		//その距離が今までで一番近い距離より近ければ
		//今一番近い敵として保存する
		if (distance < minDistance)
		{
			minDistance = distance;
			pNearestTarget = pWormEnemy;;
		}
	}

	//最も近い敵がフォーカス範囲内であればターゲットとする
	//そうでなければnullptr
	if (minDistance < focus_range)
	{
		m_pFocusTarget = pNearestTarget;
		m_isFocus = true;
	}
	else
	{
		//nullptr
		m_pFocusTarget.reset();
		m_isFocus = false;
	}


#ifdef _DEBUG
	DrawFormatString(0, 400, 0xffffff, L"ReticlePos X:%f Y:%f Z:%f",
	m_reticlePos.m_x, m_reticlePos.m_y, m_reticlePos.m_z);
	DrawFormatString(0, 420, 0xffffff, L"IsFocus:%d", m_isFocus);
#endif
}

void TargetManager::RegisterFloatingEnemy(std::shared_ptr<FloatingEnemy> pFloatingEnemy)
{
	//敵の配列に渡された敵を格納
	m_pFloatingEnemies.push_back(pFloatingEnemy);
}

void TargetManager::RegisterWormEnemy(std::shared_ptr<WormEnemy> pWormEnemy)
{
	//敵の配列に渡された敵を格納
	m_pWormEnemies.push_back(pWormEnemy);
}