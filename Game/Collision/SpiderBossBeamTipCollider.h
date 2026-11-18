#pragma once
#include "ICollider.h"

class SpiderBoss;
class SpiderBossBeamTipCollider : public ICollider {
public:
    SpiderBossBeamTipCollider(SpiderBoss& owner,bool isRight);
	virtual ~SpiderBossBeamTipCollider();
	
	std::vector<std::shared_ptr<ColliderShape>> GetCollision() const override;
    ColliderTag GetTag() const override;
    void OnCollision(const ICollider& other) override;
	bool IsCollisionActive() const override;

private:
	//当たり判定の持ち主
	SpiderBoss& m_owner;
	//ビームの先端が右側かどうか
	bool m_isRight = false;
};
