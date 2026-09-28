#include "ISpecialActionState.h"

ISpecialActionState::ISpecialActionState(
	const std::weak_ptr<Player> pPlayer) :
	IPlayerState<ISpecialActionState>(pPlayer)
{

}

