#include "GameObject.h"
#include "../../Manager/GameObjectManager.h"

GameObject::GameObject() :
	m_pos(0.0f, 0.0f, 0.0f),
	m_rotation(),
	m_isDead(false)
{

}

GameObject::~GameObject()
{

}

void GameObject::Init()
{
	//GameObjectManagerに登録する
	GameObjectManager::GetInstance().Register(shared_from_this());

	//GameObjectを継承したクラスの独自のInit処理を呼ぶ
	OnInit();
}

void GameObject::Rotate(const Vector3& axis, float angle)
{
	//クォータニオンを作ってm_rotationに掛け算(複合回転)
	Quaternion q = Quaternion(axis, angle);
	m_rotation = m_rotation * q;
}