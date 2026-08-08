#pragma once
#include <memory>
#include <vector>
#include <string>

class WormEnemy;
class Player;
class CameraBase;
class BulletManager;
class WormEnemyDataSetter
{
public:
    //敵を生成
    static std::vector<std::shared_ptr<WormEnemy>> CreateEnemy(
        std::weak_ptr<Player> pPlayer,
        std::weak_ptr<CameraBase> pCamera,
        std::weak_ptr<BulletManager> pBulletManager
    );
};

