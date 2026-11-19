#pragma once
#include "Game/GameObjects/Actors/Character/Enemy/EnemyBase.h"
#include "Manager/ResourceLoader.h"

class CameraBase;
class Player;
class BulletManager;
class SoundManager;
class EffectManager;
class BossBase : public EnemyBase
{
public:
	struct BossBaseData
	{
		ResourceLoader::ModelID modelID;
		std::weak_ptr<CameraBase> pCamera;
		std::weak_ptr<Player> pPlayer;
		std::weak_ptr<BulletManager> pBulletManager;
		std::weak_ptr<SoundManager> pSoundManager;
		std::weak_ptr<EffectManager> pEffectManager;
	};

	BossBase(BossBaseData& data);
	virtual ~BossBase();

	//ボス出現が終了しているかをセットする
	void SetIsBossAppear(bool isAppear) { m_isAppear = isAppear; }
	//ボス出現が終了しているかを返す
	bool IsBossAppear() const { return m_isAppear; }

private:

protected:
	//ボスが出現完了しているか
	//ゲームシーン側からセットさせる
	bool m_isAppear = false;
};
