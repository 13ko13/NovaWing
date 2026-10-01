#include "IShootState.h"

IShootState::IShootState(const std::weak_ptr<Player> pPlayer,
	std::weak_ptr<BulletManager> pBulletManager,
	std::weak_ptr<SoundManager> pSoundManager,
		std::weak_ptr<TargetManager> pTargetManager):
	IPlayerState<IShootState>(pPlayer),
	m_pBulletManager(pBulletManager),
	m_pSoundManager(pSoundManager),
	m_pTargetManager(pTargetManager)
{

}
