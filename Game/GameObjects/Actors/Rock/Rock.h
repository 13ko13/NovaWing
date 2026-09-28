#pragma once
#include "Actor.h"
#include "Manager/ResourceLoader.h"
#include "Game/Collision/RockCollider.h"

class CameraBase;
class Rock : public Actor
{
public:
	struct RockData
	{
		ResourceLoader::ModelID modelId;
		Vector3 pos;
		std::vector<float> sphereRadii;
		std::vector<float> sphereYOffsets;
	};

	Rock(std::weak_ptr<CameraBase> pCamera,
		const RockData& data
	);

	void OnInit() override;
	void Update() override;
	void Draw() override;

	//球の配列を返す
	std::vector<std::shared_ptr<SphereShape>> GetSpheres() const { return m_collider.GetSpheres(); }
	//当たり判定インターフェースを取得
	ICollider& GetCollider() { return m_collider; }

private:
	//当たり判定インターフェース
	RockCollider m_collider;
};