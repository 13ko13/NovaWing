#pragma once
#include "Actor.h"
#include "Manager/ResourceLoader.h"
#include "Utility/Sphere.h"

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
	std::vector<Sphere> GetSpheres() const { return m_spheres; }

private:
	//球を配列で持つ
	std::vector<Sphere> m_spheres;
};