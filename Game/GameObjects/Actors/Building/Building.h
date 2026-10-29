#pragma once
#include "Actor.h"
#include "Manager/ResourceLoader.h"

class BuildingCollider;
class ICollider;

class Building : public Actor
{
public:
	//ビル用の情報
	struct BuildingData
	{
		ResourceLoader::ModelID modelId;
		Vector3 pos;
		float rotYRadian;//ラジアン
		Vector3 scale;
		ResourceLoader::ModelID colModelID;
	};

	Building(std::weak_ptr<CameraBase> pCamera,const BuildingData& data);
	virtual ~Building();

	void OnInit() override;
	void Update() override;
	void Draw() override;

	//ビルの当たり判定取得
	ICollider& GetCollider();

private:
	//当たり判定用のモデルハンドル
	int m_colModelHandle = -1;
	//ビルの当たり判定
	std::unique_ptr<BuildingCollider> m_pCollider;
};