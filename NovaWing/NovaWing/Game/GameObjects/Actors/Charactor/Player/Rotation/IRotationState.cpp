#include "IRotationState.h"

IRotationState::IRotationState(const std::weak_ptr<Player> pPlayer) :
	IPlayerState<IRotationState>(pPlayer)
{

}