#define NOMINMAX

#include "TargetManager.h"
#include "Charactor/Player/Player.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/EnemyBase.h"
#include "Main/Application.h"

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

	//レティクルの位置を更新
	m_frontReticlePos = pPlayer->GetPos() + (-pPlayer->GetForward() * front_reticle_distance);
	m_reticlePos = pPlayer->GetPos() + (-pPlayer->GetForward() * reticle_distance);

	//死んでたら配列から削除
	m_pEnemies.erase(
	std::remove_if(m_pEnemies.begin(), m_pEnemies.end(),
		[](const std::weak_ptr<EnemyBase>& pEnemy)
		{
			return pEnemy.lock() == nullptr;
		}),
	m_pEnemies.end()
	);

	//ロック機能がOFFならフォーカスを持たずに終了する
	if (!m_isLocking)
	{
		m_pFocusTarget.reset();//ターゲットをなしとする
		m_isForcus = false;//フラグを降ろす
		return;//処理終了
	}

	//ここまで通っているということはロック機能がオンになっている
	std::shared_ptr<EnemyBase> pLockedEnemy = m_pFocusTarget.lock();
	//nullptrじゃなければ
	if (pLockedEnemy != nullptr)
	{
		//この敵がプレイヤーより前にいるかを判定
		Vector3 enemyPos = pLockedEnemy->GetPos();//敵位置
		Vector3 playerPos = m_pPlayer.lock()->GetPos();//プレイヤー位置
		//プレイヤーから敵までのベクトル
		Vector3 playerToEnemy = enemyPos - playerPos;
		//プレイヤーの前方向
		Vector3 playerForward = -m_pPlayer.lock()->GetForward();
		//プレイヤーの前ベクトルとプレイヤーから敵までのベクトルの
		//内積をとって、その値が正であればプレイヤーより正面ということになる
		bool isFlont = Vector3::Dot(playerToEnemy, playerForward) > 0.0f;

		//画面内にいるか
		bool isOnScreen = IsOnScreen(pLockedEnemy->GetPos());

		//生きているか
		bool isAlive = !pLockedEnemy->IsDead();

		//どれか一つでもfalseになっていればロックオンを行わない
		if (!isFlont ||
			!isOnScreen ||
			!isAlive)
		{
			m_pFocusTarget.reset();
			m_isForcus = false;
		}
		else
		{
			//当てはまっていればロックオンを確定させる
			return;
		}
	}

	//ここに来るのはロック機能中だが対象が未確定の場合
	//候補を探索する
	//プレイヤーより前にいて、画面内にいるものだけを候補とする
	float minDistance = FLT_MAX;
	//Candidateは候補という意味
	std::shared_ptr<EnemyBase> pNearestCandidate = nullptr;//一番近い候補
	std::vector<std::shared_ptr<EnemyBase>> pEnemies;
	for (std::weak_ptr<EnemyBase> weakEnemy : m_pEnemies)
	{
		//ポインタがヌルだったり死んでいた場合はcontinue
		std::shared_ptr<EnemyBase> pSharedEnemy = weakEnemy.lock();
		if (pSharedEnemy == nullptr) continue;
		if (pSharedEnemy->IsDead()) continue;

		//プレイヤーの前方にいない場合もcontinue
		Vector3 playerForward = -m_pPlayer.lock()->GetForward();
		Vector3 playerToEnemy = pSharedEnemy->GetPos() - m_pPlayer.lock()->GetPos();
		if (Vector3::Dot(playerForward, playerToEnemy) <= 0.0f) continue;

		//画面外の場合もcontinue
		if (!IsOnScreen(pSharedEnemy->GetPos())) continue;

		//レティクルからこの敵までのベクトル
		Vector3 reticleToEnemy = pSharedEnemy->GetPos() - m_reticlePos;
		float distance = reticleToEnemy.Length();//その距離
		if (distance < minDistance)
		{
			//一番近い距離と一番近い敵として保持する
			minDistance = distance;
			pNearestCandidate = pSharedEnemy;
		}

	}

	//その候補がnullじゃなければ
	if (pNearestCandidate != nullptr)
	{
		//その候補をターゲットとする
		m_pFocusTarget = pNearestCandidate;
		//フォーカス中とする
			m_isForcus = true;
	}
}

void TargetManager::Register(std::shared_ptr<EnemyBase> pEnemy)
{
	//配列に追加
	m_pEnemies.push_back(pEnemy);
}

void TargetManager::BeginLock()
{
	m_isLocking = true;
}

void TargetManager::EndLock()
{
	m_isLocking = false;
	m_pFocusTarget.reset(); 
	m_isForcus = false;
}

bool TargetManager::IsOnScreen(const Vector3& worldPos)
{
	//ワールド座標をスクリーン座標に変換
	VECTOR screenPos = ConvWorldPosToScreenPos(worldPos.ToDxLib());

	//ウィンドウサイズ
	const Size& wsize = Application::GetInstance().GetWindowSize();

	//スクリーン内に入っていなければfalseを返す
	if (screenPos.x < 0 || screenPos.x > wsize.m_width) return false;
	if (screenPos.y < 0 || screenPos.y > wsize.m_height) return false;
	if (screenPos.z >= 1.0f) return false;//視錐台より奥(背後)

	//それ以外はスクリーン内ということになる
	return true;
}
