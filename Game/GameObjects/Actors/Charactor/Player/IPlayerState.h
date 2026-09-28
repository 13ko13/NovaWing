#pragma once
#include <memory>

class Player;
/// <summary>
/// プレイヤーステートのインターフェース
/// </summary>
template <typename T>
class IPlayerState
{
public:
	//テンプレートクラスはcppに実装を書くことができないため
	//ヘッダに直接実装をかく
	IPlayerState(std::weak_ptr<Player> pPlayer):
		m_pPlayer(pPlayer)
	{

	}
	virtual ~IPlayerState() = default;

	virtual void Enter() = 0;//ステートに入った時
	virtual void Update() = 0;//更新処理
	virtual void Exit() = 0;//ステートから出たとき

	//次のステートを取得
	std::shared_ptr<T> GetNextState() { return m_pNextState; }
	//ステートを切り替える
	void ChangeState(std::shared_ptr<T> pNextState) { m_pNextState = pNextState; }

protected:
	std::weak_ptr<Player> m_pPlayer;
	std::shared_ptr<T> m_pNextState;
};