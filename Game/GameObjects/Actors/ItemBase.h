#pragma once

#include "Actor.h"
#include "Manager/ResourceLoader.h"

class CameraBase;
/// アイテムの基底クラス
class ItemBase : public Actor {
public:
    ItemBase(ResourceLoader::ModelID modelID,
    std::weak_ptr<CameraBase> pCamera);
    virtual ~ItemBase();
    void Update() override;
    void Draw() override;
    void OnInit() override;
};
