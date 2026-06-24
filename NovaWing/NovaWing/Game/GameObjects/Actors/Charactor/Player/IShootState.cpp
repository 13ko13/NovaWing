#include "IShootState.h"

IShootState::IShootState(const std::weak_ptr<Player> pPlayer):
	IPlayerState<IShootState>(pPlayer)
{

}
