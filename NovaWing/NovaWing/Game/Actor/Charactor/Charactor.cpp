#include "Charactor.h"

namespace
{
	constexpr int max_health = 100;//最大体力
}

Charactor::Charactor():
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
