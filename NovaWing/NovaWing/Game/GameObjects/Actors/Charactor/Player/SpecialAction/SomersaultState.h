#pragma once
#include "ISpecialActionState.h"

class SoundManager;
class SomersaultState : public ISpecialActionState
{
public:
	SomersaultState(const std::weak_ptr<Player> pPlayer,
		std::weak_ptr<SoundManager> pSoundManager);
	~SomersaultState();

	void Enter()override;//ステートに入った時
	void Update() override;//更新処理
	void Exit() override;//ステートから出たとき

private:
	//宙返りを行うための現在のフレーム数
	int m_frame = 0;

	//音のマネージャー
	std::weak_ptr<SoundManager> m_pSoundManager;
};