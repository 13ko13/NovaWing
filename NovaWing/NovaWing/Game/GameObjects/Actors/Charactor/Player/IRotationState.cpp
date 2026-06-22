#include "IRotationState.h"

IRotationState::IRotationState(const std::weak_ptr<Player> pPlayer) :
	m_pPlayer(pPlayer)
{

}