#include "Charactor.h"
#include "Game/GameObjects/Camera/CameraBase.h"

namespace
{
	constexpr int max_health = 100;//最大体力
}

Charactor::Charactor(ResourceLoader::ModelID modelID,
		std::weak_ptr<CameraBase> camera):
	Actor(modelID,camera),
	m_health(max_health),
	m_attackPower(1.0f)
	
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
	return max_health;
}


