#include "GunshipBossNetCollider.h"
#include "Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBossNetState.h"
#include "Game/GameObjects/Actors/Character/Enemy/Boss/GunshipBoss/GunshipBoss.h"
#include "Game/Collision/CapsuleShape.h"

GunshipBossNetCollider::GunshipBossNetCollider(GunshipBoss& owner) :
	m_owner(owner)
{
}

GunshipBossNetCollider::~GunshipBossNetCollider()
{
}

std::vector<std::shared_ptr<ColliderShape>> GunshipBossNetCollider::GetCollision() const
{
	//網のステートのカプセルコライダーを一つずつ取り出して
	//ColliderShapeの配列にpush_backして返す
	//IsCollisionActiveで網のステートか確認してから呼ばれる前提
	std::shared_ptr<GunshipBossNetState> netState =
		std::dynamic_pointer_cast<GunshipBossNetState>(m_owner.GetCurrentState());
	std::vector<std::shared_ptr<CapsuleShape>> capsules = netState->GetNetCapsules();
	//ColliderShapeの配列にいれる
	std::vector<std::shared_ptr<ColliderShape>> colliders;
	for (auto& capsule : capsules)
	{
		colliders.push_back(capsule);
	}
	return colliders;
}

ColliderTag GunshipBossNetCollider::GetTag() const
{
	return ColliderTag::BossNet;
}

void GunshipBossNetCollider::OnCollision(const ICollider& other)
{
}

int GunshipBossNetCollider::GetOwnerID() const
{
	return m_owner.GetID();
}

bool GunshipBossNetCollider::IsCollisionActive() const
{
	//ボスが生きていて、今のステートが網攻撃のステートかつ
	//網の判定が有効な状態だったらtrueを返す
	std::shared_ptr<GunshipBossNetState> netState =
		std::dynamic_pointer_cast<GunshipBossNetState>(m_owner.GetCurrentState());

	return !m_owner.IsDead() && netState != nullptr && netState->IsNetHitActive();
}
