#include "DisabledShootState.h"

DisabledShootState::DisabledShootState(const std::weak_ptr<Player> pPlayer,
		const std::weak_ptr<BulletManager> pBulletManager,
		const std::weak_ptr<SoundManager> pSoundManager) :
	IShootState(pPlayer,pBulletManager,pSoundManager)
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
