#include "Charactor.h"
#include "Game/GameObjects/Camera/CameraBase.h"

namespace
{
	
}

Charactor::Charactor(ResourceLoader::ModelID modelID,
		std::weak_ptr<CameraBase> camera,
		int maxHealth):
	Actor(modelID,camera),
	m_health(maxHealth),
	m_attackPower(1.0f),
	m_maxHealth(maxHealth)
	
{
}

Charactor::~Charactor()
{
	//処理なし
}

void Charactor::Update()
{
	//位置の更新
	m_pos += m_velocity;
}

int Charactor::GetMaxHealth() const
{
	return m_maxHealth;
}

bool Charactor::IsTakingDamageFrom(const DamageSource& source) const
{
	//setの中に同じ値があるかを探す
	//みつからなければend()を返す
	return m_damageSources.find(source) != m_damageSources.end();
}

void Charactor::StartTakingDamage(const DamageSource& source)
{
	//setに追加する
	m_damageSources.insert(source);
}

void Charactor::OnLeaveDamaging(const DamageSource& source)
{
	//setから削除する
	m_damageSources.erase(source);
}


