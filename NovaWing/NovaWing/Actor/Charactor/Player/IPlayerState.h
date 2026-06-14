#pragma once
#include <memory>

class Player;
/// <summary>
/// プレイヤーステートのインターフェース
/// </summary>
class IPlayerState
{
public:
	IPlayerState(std::shared_ptr<Player> pPlayer);
	virtual ~IPlayerState() = default;

	virtual void Enter() = 0;//ステートに入った時
	virtual void Update() = 0;//更新処理
	virtual void Exit() = 0;//ステートから出たとき

protected:
	std::shared_ptr<Player> m_pPlayer;
};

