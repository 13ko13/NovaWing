#pragma once
#include <memory>

#include "Manager/ResourceLoader.h"

class FloatingEnemy;
class IFloatingEnemyState
{
public:
	IFloatingEnemyState(std::weak_ptr<FloatingEnemy> pEnemy);
	virtual ~IFloatingEnemyState() = default;

	virtual void Enter() = 0;//ステートに入った時
	virtual void Update() = 0;//更新処理
	virtual void Exit() = 0;//ステートから出たとき

	//次のステートを取得
	std::shared_ptr<IFloatingEnemyState> GetNextState() { return m_nextState; }
	//ステートを変更
	void ChangeState(std::shared_ptr<IFloatingEnemyState> pNextState) { m_nextState = pNextState; }

protected:
	std::weak_ptr<FloatingEnemy> m_pEnemy;//敵自身のポインタ
	std::shared_ptr<IFloatingEnemyState> m_nextState;//次のステート
};