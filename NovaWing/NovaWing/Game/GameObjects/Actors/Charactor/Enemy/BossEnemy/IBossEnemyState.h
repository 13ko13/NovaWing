#pragma once
#include <memory>

class BossEnemy;
class IBossEnemyState
{
public:
	IBossEnemyState(std::weak_ptr<BossEnemy> pEnemy);
	virtual ~IBossEnemyState() = default;

	virtual void Enter() = 0;//ステートに入った時
	virtual void Update() = 0;//更新処理
	virtual void Exit() = 0;//ステートから出たとき
	virtual void Draw() {};//描画処理

	//次のステートを取得
	std::shared_ptr<IBossEnemyState> GetNextState() { return m_nextState; }
	//ステートを変更
	void ChangeState(std::shared_ptr<IBossEnemyState> pNextState) { m_nextState = pNextState; }

protected:
	std::weak_ptr<BossEnemy> m_pBoss;//ボス自身のポインタ
	std::shared_ptr<IBossEnemyState> m_nextState;//次のステート
};

