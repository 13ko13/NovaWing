#pragma once
#include <memory>

#include "Charactor/Player/IPlayerState.h"

class BulletManager;
class Player;
class SoundManager;
/// <summary>
/// プレイヤーの弾撃ちステートの窓口
/// </summary>
class IShootState : public IPlayerState<IShootState>
{
public:
	IShootState(const std::weak_ptr<Player> pPlayer,
		std::weak_ptr<BulletManager> pBulletManager,
		std::weak_ptr<SoundManager> pSoundManager);
	virtual ~IShootState() = default;

	virtual void Enter() = 0;//ステートに入った時
	virtual void Update() = 0;//更新処理
	virtual void Exit() = 0;//ステートから出たとき

protected:
	//弾のマネージャー
	std::weak_ptr<BulletManager> m_pBulletManager;
	//音のマネージャー
	std::weak_ptr<SoundManager> m_pSoundManager;
};

