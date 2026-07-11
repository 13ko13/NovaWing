#pragma once
#include "Actor.h"
#include "Manager/ResourceLoader.h"

class CameraBase;
class Rock : public Actor
{
public:
    Rock(ResourceLoader::ModelID modelId,
    std::weak_ptr<CameraBase> pCamera,
    const Vector3& pos);

    void OnInit() override;
    void Update() override;
    void Draw() override;
};

