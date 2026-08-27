#include "GameObject.h"
#include "../../Manager/GameObjectManager.h"

GameObject::GameObject() :
	m_pos(0.0f, 0.0f, 0.0f),
	m_rotation(),
	m_isDead(false)
{
	//一回生成されてからはずっと存在する
	static int s_nextId = 0;
	//それ以降はインクリメントされた番号が入る
	m_id = s_nextId;
	s_nextId++;
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

Vector3 GameObject::GetForward() const
{
	//前方向のベクトル
	const Vector3 forward = Vector3(0.0f, 0.0f,1.0f);
	//このベクトルをそれぞれが持っているm_rotationを使用して
	//回転させたベクトルを返す
	const Vector3 rotForward = m_rotation * forward;
	return rotForward;
}

Vector3 GameObject::GetRight() const
{
	//右方向のベクトル
	const Vector3 right = Vector3(1.0f, 0.0f, 0.0f);
	//このベクトルをそれぞれが持っているm_rotationを使用して
	//回転させたベクトルを返す
	const Vector3 rotRight = m_rotation * right;
	return rotRight;
}

Vector3 GameObject::GetLeft() const
{
	//左方向のベクトル
	const Vector3 left = Vector3(-1.0f, 0.0f, 0.0f);
	//このベクトルをそれぞれが持っているm_rotationを使用して
	//回転させたベクトルを返す
	const Vector3 rotLeft = m_rotation * left;
	return rotLeft;
}

Vector3 GameObject::GetBack() const
{
	//後ろ方向のベクトル
	const Vector3 back = Vector3(0.0f, 0.0f, -1.0f);
	//このベクトルをそれぞれが持っているm_rotationを使用して
	//回転させたベクトルを返す
	const Vector3 rotBack = m_rotation * back;
	return rotBack;
}
