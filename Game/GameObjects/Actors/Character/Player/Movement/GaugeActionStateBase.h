#pragma once
#include "IMovementState.h"
#include <memory>
class SoundManager;
class GaugeActionStateBase : public IMovementState
{
public:
	GaugeActionStateBase(const std::weak_ptr<Player> pPlayer,
		std::weak_ptr<SoundManager> pSoundManager);
	~GaugeActionStateBase();

	void Update() override;//更新処理
	//純粋仮想
	virtual void Enter() = 0;//ステートに入った時
	virtual void Exit() = 0;//ステートから出たとき

private:

protected:
	virtual float GetSpeed() const = 0;//速度
	virtual const char* GetButtonName() const = 0;//ボタン名

	//音のマネージャー
	std::weak_ptr<SoundManager> m_pSoundManager;
};