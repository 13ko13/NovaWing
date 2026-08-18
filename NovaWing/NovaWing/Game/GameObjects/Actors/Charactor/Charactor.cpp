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


