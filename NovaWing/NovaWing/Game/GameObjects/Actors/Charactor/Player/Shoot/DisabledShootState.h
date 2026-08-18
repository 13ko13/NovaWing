#pragma once
#include "IShootState.h"

class BulletManager;
class DisabledShootState : public IShootState
{
public:
	DisabledShootState(const std::weak_ptr<Player> pPlayer,
		const std::weak_ptr<BulletManager> pBulletManager);
	~DisabledShootState();

	void Enter()override;//ステートに入った時
	void Update() override;//更新処理
	void Exit() override;//ステートから出たとき

private:

};

