#pragma once
#include <memory>

class SpiderBoss;
class ISpiderBossState
{
public:
	ISpiderBossState(std::weak_ptr<SpiderBoss> pEnemy);
	virtual ~ISpiderBossState() = default;

	virtual void Enter() = 0;//ステートに入った時
	virtual void Update() = 0;//更新処理
	virtual void Exit() = 0;//ステートから出たとき
	virtual void Draw() {};//描画処理

	//次のステートを取得
	std::shared_ptr<ISpiderBossState> GetNextState() { return m_nextState; }
	//ステートを変更
	void ChangeState(std::shared_ptr<ISpiderBossState> pNextState) { m_nextState = pNextState; }

protected:
	std::weak_ptr<SpiderBoss> m_pBoss;//ボス自身のポインタ
	std::shared_ptr<ISpiderBossState> m_nextState;//次のステート
};

