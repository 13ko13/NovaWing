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
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();
	if (pPlayer != nullptr)
	{
		pPlayer->AddDefeatedEnemyCount();

		//何で倒されたかも伝える(チュートリアルの判定用)
		switch (m_lastDamageSource)
		{
		case DamageSource::NormalShot:
			pPlayer->AddNormalShotDefeatedCount();
			break;
		case DamageSource::ChargeShot:
			pPlayer->AddChargeShotDefeatedCount();
			break;
		default:
			break;
		}
	}

	OnDead();
}
