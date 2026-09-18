#include "ItemBase.h"

ItemBase::ItemBase(ResourceLoader::ModelID modelID,
	std::weak_ptr<CameraBase> pCamera) :
	Actor(modelID,pCamera)
{
}

ItemBase::~ItemBase() {
}

void ItemBase::Update() {
}

void ItemBase::Draw() {
}

void ItemBase::OnInit()
{
}
