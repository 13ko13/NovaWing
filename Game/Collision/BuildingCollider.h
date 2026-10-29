#pragma once

#include <memory>
#include <vector>

#include "ICollider.h"

class Building;
class MeshShape;

class BuildingCollider : public ICollider
{
public:
    BuildingCollider(Building& owner,int colModelHandle);
	virtual ~BuildingCollider();

	//当たり判定を取得
	std::vector<std::shared_ptr<ColliderShape>> GetCollision() const override;
	//タグを取得
	ColliderTag GetTag() const override { return ColliderTag::Building; }
	//当たり判定が有効かを返す
	bool IsCollisionActive() const override;
	//衝突時処理
	void OnCollision(const ICollider& other) override;
	//持ち主のIDを取得(多段ヒット防止の識別に使う)
    int GetOwnerID() const;

private:
	//当たり判定を必要とするオブジェクト
	Building& m_owner;
	//当たり判定用のメッシュ
	std::shared_ptr<MeshShape> m_mesh;
};
