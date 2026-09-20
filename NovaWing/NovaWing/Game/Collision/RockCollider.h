#pragma once

#include "ICollider.h"
#include "Utility/Vector3.h"
#include "SphereShape.h"

class Rock;

/// 岩の当たり判定
class RockCollider : public ICollider 
{
public:
    RockCollider(Rock& owner);
    ~RockCollider();

    //当たり判定の形状データ取得
    std::vector<std::shared_ptr<ColliderShape>> GetCollision() const override;
    //自分のタグを取得
    ColliderTag GetTag() const override;
    //衝突時処理
    void OnCollision(const ICollider& other) override;
    //判定が有効か
    bool IsCollisionActive() const override;

    //球を一つ追加する
    void AddSphere(const Vector3& pos, float radius);
    std::vector<std::shared_ptr<SphereShape>> GetSpheres() const { return m_spheres; }

private:
    //岩を参照したい
    Rock& m_owner;
    //当たり判定用の球の配列
    std::vector<std::shared_ptr<SphereShape>> m_spheres;
};