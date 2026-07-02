#include "IShootState.h"

IShootState::IShootState(const std::weak_ptr<Player> pPlayer,
	std::weak_ptr<BulletManager> pBulletManager):
	IPlayerState<IShootState>(pPlayer),
	m_pBulletManager(pBulletManager)
{

}
