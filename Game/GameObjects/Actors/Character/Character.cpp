#include "Character.h"
#include "Game/GameObjects/Camera/CameraBase.h"

namespace
{
	
}

Character::Character(ResourceLoader::ModelID modelID,
		std::weak_ptr<CameraBase> camera,
		int maxHealth):
	Actor(modelID,camera),
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
	m_pos += m_velocity;
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


