#pragma once

#include "Manager/ResourceLoader.h"
#include "Game/GameObjects/Actors/Actor.h"
#include "Utility/Sphere.h"

class BulletBase : public Actor
{
public:
	BulletBase(const Vector3& pos,const Vector3& vel,int attackPower,
		ResourceLoader::ModelID modelId);
	virtual ~BulletBase();

	virtual void Update();//更新処理
	virtual void Draw();//描画処理

	//当たり判定用の球を取得
	const Sphere& GetSphere() const { return m_sphere; }

	//攻撃力を取得
	const int GetAttackPower() const { return m_attackPower; }

protected:
	//当たり判定用の球
	Sphere m_sphere;

	//攻撃力
	int m_attackPower;
	//生存時間
	int m_lifeTime;
};

