#include "IShootState.h"

IShootState::IShootState(const std::weak_ptr<Player> pPlayer,
	std::weak_ptr<BulletManager> pBulletManager,
	std::weak_ptr<SoundManager> pSoundManager):
	IPlayerState<IShootState>(pPlayer),
	m_pBulletManager(pBulletManager),
	m_pSoundManager(pSoundManager)
{

}
