#pragma once
#include "Charactor.h"
class Player : public Charactor
{
public:
	Player();

private:
	//ブーストゲージ
	int m_boostGauge = 100;
	float m_boostSpeed = 0;
	float m_brakeSpeed = 0;
};

