#pragma once
#include <memory>

class Player;
/// <summary>
/// プレイヤーの移動ステートの窓口
/// </summary>
class IMovementState
{
public:
	IMovementState(const std::weak_ptr<Player> pPlayer);
	virtual ~IMovementState() = default;

	virtual void Enter() = 0;//ステートに入った時
	virtual void Update() = 0;//更新処理
	virtual void Exit() = 0;//ステートから出たとき

	/// <summary>
	/// 次のステートを返す
	/// </summary>
	/// <returns>次のステート</returns>
	std::shared_ptr<IMovementState> GetNextState() { return m_pNextState; }

	/// <summary>
	/// 次のステートをセットする
	/// </summary>
	/// <param name="pNextState">次のステート</param>
	void ChangeState(std::shared_ptr<IMovementState> pNextState) { m_pNextState = pNextState; }

protected:
	std::weak_ptr<Player> m_pPlayer;

	std::shared_ptr<IMovementState> m_pNextState;
};