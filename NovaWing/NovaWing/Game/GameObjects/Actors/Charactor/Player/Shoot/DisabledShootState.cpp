#include "DisabledShootState.h"

DisabledShootState::DisabledShootState(const std::weak_ptr<Player> pPlayer,
		const std::weak_ptr<BulletManager> pBulletManager,
		const std::weak_ptr<SoundManager> pSoundManager, 
	std::weak_ptr<TargetManager> pTargetManager) :
	IShootState(pPlayer,pBulletManager,pSoundManager,pTargetManager)
{
}

DisabledShootState::~DisabledShootState()
{
}

void DisabledShootState::Enter()
{
}

void DisabledShootState::Update()
{
}

void DisabledShootState::Exit()
{
}
