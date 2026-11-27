#pragma once
#include "Game/GameObjects/Actors/Actor.h"

/// <summary>
/// ロケットの見た目
/// </summary>
class RocketModel : public Actor
{
public:
	RocketModel(std::weak_ptr<CameraBase> pCamera);
	virtual ~RocketModel();

	//更新
	void Update() override;
	//描画
	void Draw() override;

	//進む方向に機首を向ける
	void LookAtDir(const Vector3& dir);

private:

protected:
};
