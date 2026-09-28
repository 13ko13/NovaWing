#pragma once
#include <set>

#include "../Actor.h"
#include "DamageSource.h"

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

	//既にそのダメージ源から受けているか
	bool IsTakingDamageFrom(const DamageSource& source) const;
	//そのダメージ源から受け始めたことを記録する
	void StartTakingDamage(const DamageSource& source);
	//そのダメージ源から受けなくなったことを記録する
	void OnLeaveDamaging(const DamageSource& source);

protected:
	//機体情報
	int m_health = 100;//体力
	float m_attackPower = 1.0f;//攻撃力
	//最大体力
	int m_maxHealth = 100;

	//ダメージの発生源のセット
	std::set<DamageSource> m_damageSources;

protected:
	
};