#include "CounterCollider.h"
#include "Game/GameObjects/Actors/Character/Player/Player.h"

namespace
{
	//カウンター後の無敵時間
	constexpr int counter_invincible_frame = 120;
}

CounterCollider::CounterCollider(Player& owner) :
	m_owner(owner)
{
	m_sphere = std::make_shared<SphereShape>();
}

CounterCollider::~CounterCollider()
{
}

std::vector<std::shared_ptr<ColliderShape>> CounterCollider::GetCollision() const
{
	return { m_sphere };
}

void CounterCollider::OnCollision(const ICollider& other)
{
	//相手のタグがボスビームの先端か、反射後のボスビームの時だけ処理
	if (other.GetTag() != ColliderTag::BossBeamTip &&
		other.GetTag() !=ColliderTag::BossBeamReflect) return;

	//無敵時間を開始する
	m_owner.OnInvincibleStart(counter_invincible_frame);
}

void CounterCollider::UpdateShape(const Vector3& pos, float radius)
{
	m_sphere->Update(pos, radius);
}

bool CounterCollider::IsCollisionActive() const
{
	//ローリング中のみtrueを返す
	return m_owner.IsRolling();
}