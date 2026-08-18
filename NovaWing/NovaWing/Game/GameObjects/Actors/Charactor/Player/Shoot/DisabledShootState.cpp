#include "DisabledShootState.h"

DisabledShootState::DisabledShootState(const std::weak_ptr<Player> pPlayer,
		const std::weak_ptr<BulletManager> pBulletManager) :
	IShootState(pPlayer,pBulletManager)
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
