#include "Actor.h"
#include "../../Manager/ActorManager.h"

Actor::Actor() :
	m_pos(0.0f, 0.0f, 0.0f),
	m_rotation(),
	m_velocity(0.0f, 0.0f, 0.0f),
	m_isDead(false)
{
	
}

Actor::~Actor()
{
	//処理なし
}

void Actor::Init()
{
	//ActorManagerに登録する
	ActorManager::GetInstance().Register(shared_from_this());
	//Actorを継承したクラスの独自のInit処理を呼ぶ
	OnInit();
}

void Actor::Rotate(const Vector3& axis, float angle)
{
	//クォータニオンを作ってm_rotationに掛け算(複合回転)
	Quaternion q = Quaternion(axis, angle);
	m_rotation = m_rotation * q;
}
