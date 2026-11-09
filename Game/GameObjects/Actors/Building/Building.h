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
		bool isLoop = false;//ループするか
		float oneLapDistance = 0.0f;//1周の長さ
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

	//このビルがループ対象か
	bool m_isLoop = false;
	//ボス登場時からのステージ1周分の長さ
	float m_oneLapDistance = 0.0f;
	//最初にビルを配置した位置
	float m_baseZ = 0.0f;
};