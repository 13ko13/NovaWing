#pragma once

#include "Manager/ResourceLoader.h"

class Vector3;
class BulletBase
{
public:
	BulletBase(const Vector3& pos,const Vector3& vel,int attackPower,
		ResourceLoader::ModelID modelId);
	virtual ~BulletBase();

	virtual void Update() = 0;
	virtual void Draw() = 0;
};

