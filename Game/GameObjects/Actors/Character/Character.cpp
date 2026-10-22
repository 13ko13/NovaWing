#include "Character.h"
#include "Game/GameObjects/Camera/CameraBase.h"
#include "Main/Application.h"

namespace
{

}

Character::Character(ResourceLoader::ModelID modelID,
		std::weak_ptr<CameraBase> camera,
		int maxHealth) :
	Actor(modelID, camera),
	m_health(maxHealth),
	m_attackPower(1.0f),
	m_maxHealth(maxHealth)

{
}

Character::~Character()
{
	//処理なし
}

void Character::Update()
{
	//位置の更新
	//タイムスケールの対象かどうかで進む速度を変える
	float timeScale = Application::GetInstance().GetTimeScale();
	if (IsAffectedByTimeScale())
	{
		m_pos += m_velocity * timeScale;
	}
	else
	{
		m_pos.x += m_velocity.x;
		m_pos.y += m_velocity.y;
		m_pos.z += m_velocity.z * timeScale;
	}
}

int Character::GetMaxHealth() const
{
	return m_maxHealth;
}

bool Character::IsTakingDamageFrom(const DamageSource& source) const
{
	//setの中に同じ値があるかを探す
	//みつからなければend()を返す
	return m_damageSources.find(source) != m_damageSources.end();
}

void Character::StartTakingDamage(const DamageSource& source)
{
	//setに追加する
	m_damageSources.insert(source);
}

void Character::OnLeaveDamaging(const DamageSource& source)
{
	//setから削除する
	m_damageSources.erase(source);
}


