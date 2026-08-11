#pragma once
#include <vector>
#include <memory>

class BossEnemy;
class Player;
class CameraBase;
class BulletManager;
class EnemyFactory;
class BossEnemyDataSetter
{
public:
    //敵を生成
    static std::shared_ptr<BossEnemy> CreateEnemy(
        std::weak_ptr<Player> pPlayer,
        std::weak_ptr<CameraBase> pCamera,
        std::weak_ptr<BulletManager> pBulletManager
    );
};

