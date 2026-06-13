#pragma once
#include "Actor.h"

class Charactor : public Actor
{
public:
	Charactor();
	virtual ~Charactor();
	virtual void Update() override;
	virtual void Draw() = 0;
	virtual void TakeDamage(int damage) = 0;//ダメージを受ける

	int GetHealth() const { return m_health; }//HPを取得

protected:
	int m_health = 100;//体力
	float m_attackPower = 1.0f;//攻撃力
};

