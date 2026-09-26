#pragma once

#include "BulletBase.h"

class ReflectedBullet : public BulletBase {
public:
    struct ReflectBulletData
    {
        Vector3 pos;
        Vector3 vel;
        int attackPower;
        std::weak_ptr<GameObject> pTarget;
        float homingStrength;
        std::weak_ptr<CameraBase> pCamera;
    };
    ReflectedBullet(ReflectBulletData& data);
    virtual ~ReflectedBullet();

    void Update() override;
    void Draw() override;
    //敵にヒットした時の処理
    void OnHitEnemy() override;

private:
    //ホーミング先のターゲット
    std::weak_ptr<GameObject> m_pTarget;
    //ホーミングの強さ
    float m_homingStrength = 0.0f;
    //受け取った弾の速度
    float m_speed = 0.0f;
    //Effekseerエフェクト再生ハンドル
    int m_effectPlayH = -1;
};
