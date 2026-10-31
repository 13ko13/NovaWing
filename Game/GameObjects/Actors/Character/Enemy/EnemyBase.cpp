#include "EnemyBase.h"
#include "Game/GameObjects/Actors/Character/Player/Player.h"
#include "CSVData/EnemyStatusData.h"
#include "Manager/SoundManager.h"
#include "Manager/EffectManager.h"

EnemyBase::EnemyBase(
	ResourceLoader::ModelID modelID,
	std::weak_ptr<CameraBase> pCamera,
	std::weak_ptr<Player> pPlayer,
	std::weak_ptr<BulletManager> pBulletManager,
	std::weak_ptr<SoundManager> pSoundManager,
	std::weak_ptr<EffectManager> pEffectManager) :
	Character(modelID, pCamera, EnemyStatusData::FindByModelID(modelID).GetHp()),
	m_pPlayer(pPlayer),
	m_pBulletManager(pBulletManager),
	m_pSoundManager(pSoundManager),
	m_pEffectManager(pEffectManager)
{
	//modelIDをキーにデータを探して受け取る
	m_colRadius = EnemyStatusData::FindByModelID(modelID).GetColRadius();
	m_trueDeadFrame = EnemyStatusData::FindByModelID(modelID).GetTrueDeadFrame();
}

EnemyBase::~EnemyBase()
{
}

void EnemyBase::TakeDamage(int damage)
{
	//死亡待機中は死亡処理を二重に行わない
	if (m_isDying) return;

	//体力を減らす
	m_health -= damage;
	//体力が0以下になったら死亡処理
	if (m_health <= 0)
	{
		//死亡待機状態をtrueにする
		m_isDying = true;

		//死亡音を鳴らす
		m_pSoundManager.lock()->Play(SoundManager::SoundType::EnemyDeath);

		//死亡エフェクトの再生を依頼する
		m_pEffectManager.lock()->PlayOneShot(
			ResourceLoader::EffectID::FloatingDeath, m_pos);
	}
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
