#pragma once
#include <memory>

#include "ICollider.h"
#include "SphereShape.h"

class Player;
class CounterCollider : public ICollider
{
public:
    CounterCollider(Player& owner);
    virtual ~CounterCollider();

    //コライダー取得
    std::vector<std::shared_ptr<ColliderShape>> GetCollision() const override;
    //タグ取得
    ColliderTag GetTag() const override { return ColliderTag::Counter; }
    //当たったときの処理
    void OnCollision(const ICollider& other) override;
    //コライダーが有効か
    bool IsCollisionActive() const override;
    //球そのものを取得
    std::shared_ptr<SphereShape> GetSphere() const { return m_sphere; }
    //球の位置、半径を更新
    void UpdateShape(const Vector3& pos, float radius);

private:
    //プレイヤーを参照したい
    Player& m_owner;
    //当たり判定用の球
    std::shared_ptr<SphereShape> m_sphere;
};