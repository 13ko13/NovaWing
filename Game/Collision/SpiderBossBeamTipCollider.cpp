#include "DxLib.h"
#include "SpiderBossBeamTipCollider.h"
#include "Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBoss.h"
#include "Game/GameObjects/Actors/Character/Enemy/Boss/SpiderBoss/SpiderBossBeamState.h"
#include "Game/Collision/CounterCollider.h"

SpiderBossBeamTipCollider::SpiderBossBeamTipCollider(SpiderBoss& owner, bool isRight) :
	m_owner(owner),
	m_isRight(isRight)
{
}

SpiderBossBeamTipCollider::~SpiderBossBeamTipCollider()
{
}

std::vector<std::shared_ptr<ColliderShape>> SpiderBossBeamTipCollider::GetCollision() const
{
	//現在のステートがボスビームステートか
	std::shared_ptr<SpiderBossBeamState> pBeamState = std::dynamic_pointer_cast<SpiderBossBeamState>(m_owner.GetCurrentState());
	if(!pBeamState)
	{
		return {};
	}

	//自分が右なら右のビームの先端球を返す
	if (m_isRight)
	{
		return { pBeamState->GetTipSphereR() };
	}
	else
	{
		return { pBeamState->GetTipSphereL() };
	}
}

ColliderTag SpiderBossBeamTipCollider::GetTag() const
{
	if(m_isRight)
	{
		//BossBeamStateにキャストして、右のビームが反射されているかを取得する
		std::shared_ptr<SpiderBossBeamState> pBeamState = std::dynamic_pointer_cast<SpiderBossBeamState>(m_owner.GetCurrentState());
		if (!pBeamState) return ColliderTag::BossBeamTip;
		
		if(pBeamState->IsReflectedR())
		{
			return ColliderTag::BossBeamReflect;
		}
		else return ColliderTag::BossBeamTip;
	}
	else
	{
		//BossBeamStateにキャストして、左のビームが反射されているかを取得する
		std::shared_ptr<SpiderBossBeamState> pBeamState = std::dynamic_pointer_cast<SpiderBossBeamState>(m_owner.GetCurrentState());
		if (!pBeamState) return ColliderTag::BossBeamTip;

		if (pBeamState->IsReflectedL())
		{
			return ColliderTag::BossBeamReflect;
		}
		else return ColliderTag::BossBeamTip;
	}
}

void SpiderBossBeamTipCollider::OnCollision(const ICollider& other)
{
	//現在のステートがボスビームステートか
	std::shared_ptr<SpiderBossBeamState> pBeamState = std::dynamic_pointer_cast<SpiderBossBeamState>(m_owner.GetCurrentState());
	if (!pBeamState) return;

	//相手のタグがカウンターの時だけ処理
	if (other.GetTag() == ColliderTag::Counter)
	{
		//カウンター球の中心を取得
		Vector3 counterPos = static_cast<const CounterCollider&>(other).GetSphere()->GetPos();
	
		//ボスビームステートに右/左のビームが反射されたことを通知する
		if(m_isRight)
		{
			pBeamState->OnReflectRight(counterPos);
		}
		else
		{
			pBeamState->OnReflectLeft(counterPos);
		}
	}
	if (other.GetTag() == ColliderTag::BossDamage)
	{
		//右/左のビームに当たったことを知らせる
		if (m_isRight) pBeamState->OnHitBossR();
		if (!m_isRight) pBeamState->OnHitBossL();
	}
	
}

bool SpiderBossBeamTipCollider::IsCollisionActive() const
{
	//ボスが生きているか
	bool isAlive = !m_owner.IsDead();
	//今のステートがBossBeamStateか
	std::shared_ptr<SpiderBossBeamState> pBeamState = std::dynamic_pointer_cast<SpiderBossBeamState>(m_owner.GetCurrentState());
	//dynamic_castで、キャストできれば現在はBossBeamStateであることがわかる
	bool isBeamState = pBeamState != nullptr;
	//ビームステートではない時点で処理を飛ばす
	if (!isBeamState) return false;
	
	//まだ右/左側のビームが反射されていないか
	bool isHitBoss = false;
	if (m_isRight)
	{
		isHitBoss = pBeamState->IsHitBossR();
	}
	else
	{
		isHitBoss = pBeamState->IsHitBossL();
	}

	return isAlive && isBeamState && !isHitBoss;
}
