#pragma once
#include "Game/GameObjects/Actors/Actor.h"
#include "Manager/ResourceLoader.h"

/// <summary>
/// 景色として置くだけのオブジェクト(島や岩など)
/// 当たり判定を持たず、プレイヤーや敵には一切影響しない
/// </summary>
class CameraBase;
class Scenery : public Actor
{
public:
	struct SceneryData
	{
		ResourceLoader::ModelID modelId;
		Vector3 pos;
		float rotYRadian;
		Vector3 scale;
	};

	Scenery(std::weak_ptr<CameraBase> pCamera, const SceneryData& data);

	void OnInit() override;
	void Update() override;
	void Draw() override;

private:
	//CSVに書かれたz座標(ループ1周分の中での位置)
	float m_baseZ;
};
