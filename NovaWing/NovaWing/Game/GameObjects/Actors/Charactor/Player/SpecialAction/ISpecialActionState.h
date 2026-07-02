#pragma once
#include <memory>

#include "Charactor/Player/IPlayerState.h"

class Player;
/// <summary>
/// プレイヤーの特殊アクションステートの窓口
/// </summary>
class ISpecialActionState : public IPlayerState<ISpecialActionState>
{
public:
	ISpecialActionState(const std::weak_ptr<Player> pPlayer);
	virtual ~ISpecialActionState() = default;

	virtual void Enter() = 0;//ステートに入った時
	virtual void Update() = 0;//更新処理
	virtual void Exit() = 0;//ステートから出たとき

protected:

};