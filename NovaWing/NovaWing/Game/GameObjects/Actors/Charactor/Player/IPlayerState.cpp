#include "IPlayerState.h"

IPlayerState::IPlayerState(std::shared_ptr<Player> pPlayer):
	m_pPlayer(pPlayer)
{
}