#include "EnemyBase.h"
#include "Game/GameObjects/Actors/Character/Player/Player.h"
#include "CSVData/EnemyStatusData.h"

EnemyBase::EnemyBase(
	ResourceLoader::ModelID modelID,
	std::weak_ptr<CameraBase> pCamera,
	std::weak_ptr<Player> pPlayer,
	std::weak_ptr<BulletManager> pBulletManager) :
	Character(modelID, pCamera, EnemyStatusData::FindByModelID(modelID).GetHp()),
	m_pPlayer(pPlayer),
	m_pBulletManager(pBulletManager)
{
	//modelIDをキーにデータを探して受け取る
	m_colRadius = EnemyStatusData::FindByModelID(modelID).GetColRadius();
	m_trueDeadFrame = EnemyStatusData::FindByModelID(modelID).GetTrueDeadFrame();
}

EnemyBase::~EnemyBase()
{
}

void EnemyBase::OnEnemyDead()
{
	//プレイヤーに敵が死んだことを伝える
	if (m_pPlayer.lock() != nullptr)
	{
		m_pPlayer.lock()->AddDefeatedEnemyCount();	
	}

	OnDead();
}
