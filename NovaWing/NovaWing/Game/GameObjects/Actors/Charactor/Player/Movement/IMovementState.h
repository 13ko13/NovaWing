#pragma once
#include <memory>

#include "Charactor/Player/IPlayerState.h"

class Player;
/// <summary>
/// プレイヤーの移動ステートの窓口
/// </summary>
class IMovementState : public IPlayerState<IMovementState>
{
public:
	IMovementState(const std::weak_ptr<Player> pPlayer);
	virtual ~IMovementState() = default;

	virtual void Enter() = 0;//ステートに入った時
	virtual void Update() = 0;//更新処理
	virtual void Exit() = 0;//ステートから出たとき

protected:
	
};