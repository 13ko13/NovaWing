#pragma once
#include "IShootState.h"

class Player;
class BulletManager;
class SoundManager;
class NormalShootState : public IShootState
{
  public:
	NormalShootState(
		const std::weak_ptr<Player> pPlayer,
		std::weak_ptr<BulletManager> pBulletManager,
		std::weak_ptr<SoundManager> pSoundManager);
	~NormalShootState();

	void Exit() override;
	void Update() override;
	void Enter() override;

  private:
	// クールタイム管理
	int m_shootCT = 0;
	// どのぐらいショットボタンを押しているか
	int m_pressingShootButton = 0;
};
