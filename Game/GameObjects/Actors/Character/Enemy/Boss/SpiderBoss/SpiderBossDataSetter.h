#pragma once
#include <vector>
#include <memory>
#include <string>

class SpiderBoss;
class Player;
class CameraBase;
class BulletManager;
class EnemyFactory;
class SoundManager;
class EffectManager;
class SpiderBossDataSetter
{
public:
    //敵を生成
    static std::shared_ptr<SpiderBoss> CreateEnemy(
        std::weak_ptr<Player> pPlayer,
        std::weak_ptr<CameraBase> pCamera,
        std::weak_ptr<BulletManager> pBulletManager,
        std::weak_ptr<SoundManager> pSoundManager,
        std::weak_ptr<EffectManager> pEffectManager,
        const std::wstring& csvFolderName
    );
};

