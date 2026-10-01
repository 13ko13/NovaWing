#pragma once
#include "ICollider.h"

class BossEnemy;
class BossBeamTipCollider : public ICollider {
public:
    BossBeamTipCollider(BossEnemy& owner,bool isRight);
	virtual ~BossBeamTipCollider();
	
	std::vector<std::shared_ptr<ColliderShape>> GetCollision() const override;
    ColliderTag GetTag() const override;
    void OnCollision(const ICollider& other) override;
	bool IsCollisionActive() const override;

private:
	//当たり判定の持ち主
	BossEnemy& m_owner;
	//ビームの先端が右側かどうか
	bool m_isRight = false;
};
