#pragma once
#include <memory>

class GunshipBoss;
class IGunshipBossState
{
public:
	IGunshipBossState(std::weak_ptr<GunshipBoss> pEnemy);
	virtual ~IGunshipBossState();

	virtual void Enter() = 0;//ステートに入った時
	virtual void Update() = 0;//更新処理
	virtual void Exit() = 0;//ステートから出たとき
	virtual void Draw() {};//描画処理

	//次のステートを取得
	std::shared_ptr<IGunshipBossState> GetNextState() { return m_nextState; }
	//ステートを変更
	void ChangeState(std::shared_ptr<IGunshipBossState> pNextState) { m_nextState = pNextState; }

protected:
	std::weak_ptr<GunshipBoss> m_pBoss;//ボス自身のポインタ
	std::shared_ptr<IGunshipBossState> m_nextState;//次のステート
};
