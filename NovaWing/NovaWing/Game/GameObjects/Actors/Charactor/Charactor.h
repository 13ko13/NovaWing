#pragma once
#include "../Actor.h"

class CameraBase;
/// <summary>
/// HPを持つキャラクター
/// </summary>
class Charactor : public Actor
{
public:
	Charactor(ResourceLoader::ModelID modelID,
		std::weak_ptr<CameraBase> camera,
		int maxHealth = 100);
	virtual ~Charactor();
	virtual void Update() override;
	virtual void Draw() = 0;
	virtual void TakeDamage(int damage) = 0;//ダメージを受ける

	//HP
	int GetHealth() const { return m_health; }//HPを取得
	//最大HPを取得
	int GetMaxHealth() const;

protected:
	//機体情報
	int m_health = 100;//体力
	float m_attackPower = 1.0f;//攻撃力
	//最大体力
	int m_maxHealth = 100;

protected:
	
};