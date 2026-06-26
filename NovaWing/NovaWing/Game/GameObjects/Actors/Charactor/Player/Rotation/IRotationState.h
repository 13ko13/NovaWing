#pragma once
#include <memory>

#include "Charactor/Player/IPlayerState.h"

class Player;
/// <summary>
/// プレイヤーの回転系ステートの窓口
/// </summary>
class IRotationState : public IPlayerState<IRotationState>
{
public:
	IRotationState(const std::weak_ptr<Player> pPlayer);
	virtual ~IRotationState() = default;

	virtual void Enter() = 0;//ステートに入った時
	virtual void Update() = 0;//更新処理
	virtual void Exit() = 0;//ステートから出たとき

protected:
	
};