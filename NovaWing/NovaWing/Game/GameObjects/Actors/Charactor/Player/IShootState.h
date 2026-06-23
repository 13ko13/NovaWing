#pragma once
#include <memory>

#include "IPlayerState.h"

class Player;
/// <summary>
/// プレイヤーの弾撃ちステートの窓口
/// </summary>
class IShootState : public IPlayerState<IShootState>
{
public:
	IShootState(const std::weak_ptr<Player> pPlayer);
	virtual ~IShootState() = default;

	virtual void Enter() = 0;//ステートに入った時
	virtual void Update() = 0;//更新処理
	virtual void Exit() = 0;//ステートから出たとき

protected:

};

