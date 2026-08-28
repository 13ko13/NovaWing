#pragma once
#include "IShootState.h"

class BulletManager;
class SoundManager;
class DisabledShootState : public IShootState
{
public:
	DisabledShootState(const std::weak_ptr<Player> pPlayer,
		const std::weak_ptr<BulletManager> pBulletManager,
		const std::weak_ptr<SoundManager> pSoundManager,
		std::weak_ptr<TargetManager> pTargetManager);
	~DisabledShootState();

	void Enter()override;//ステートに入った時
	void Update() override;//更新処理
	void Exit() override;//ステートから出たとき

private:

};

