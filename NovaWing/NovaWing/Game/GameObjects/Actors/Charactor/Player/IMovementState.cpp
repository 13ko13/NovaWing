#include "IMovementState.h"

IMovementState::IMovementState(const std::weak_ptr<Player> pPlayer) :
	m_pPlayer(pPlayer)
{
}