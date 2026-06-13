#pragma once
#include <memory>

#include "IPlayerState.h"

class InputManager;
class NormalState : public IPlayerState
{
public:
	NormalState(std::shared_ptr<InputManager> pInputManager,
		std::shared_ptr<Player> pPlayer);
	~NormalState();

	void Enter() override;//入った時
	void Update() override;//更新処理
	void Exit() override;//出たとき

private:
	std::weak_ptr<InputManager> m_pInputManager;
};

