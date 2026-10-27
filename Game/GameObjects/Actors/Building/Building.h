#pragma once
#include "Actor.h"
#include "Manager/ResourceLoader.h"


class Building : public Actor
{
public:
	struct BuildingData
	{
		ResourceLoader::ModelID modelId;
		Vector3 pos;
		Vector3 rot;
		Vector3 scale;
		Vector3 colModelID;
	};

	Building(const BuildingData& data);
    virtual ~Building();
};