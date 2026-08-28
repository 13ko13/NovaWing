#pragma once
#include <memory>
#include <vector>
#include <string>

class EnemyBase;
class Player;
class CameraBase;
class BulletManager;
class SoundManager;
class FloatingEnemyDataSetter
{
public:
    //敵を生成
    static std::vector<std::shared_ptr<EnemyBase>> CreateEnemy(
        std::weak_ptr<Player> pPlayer,
        std::weak_ptr<CameraBase> pCamera,
        std::weak_ptr<BulletManager> pBulletManager,
        std::weak_ptr<SoundManager> pSoundManager
    );

private:
    
};

