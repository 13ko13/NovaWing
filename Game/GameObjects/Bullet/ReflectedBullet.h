#pragma once

#include "BulletBase.h"

class EnemyBase;
class RocketModel;
class ReflectedBullet : public BulletBase
{
public:
    struct ReflectBulletData
    {
        Vector3 pos;
        Vector3 vel;
        int attackPower;
        std::weak_ptr<EnemyBase> pTarget;
        float homingStrength;
		std::weak_ptr<CameraBase> pCamera;
		bool isDamageReduced = true;
		std::shared_ptr<RocketModel> pModel;
		ResourceLoader::EffectID hitEffectID = ResourceLoader::EffectID::HitEffect;
	};
    ReflectedBullet(const ReflectBulletData& data, std::weak_ptr<EffectManager> pEffectManager);
    virtual ~ReflectedBullet();

    void Update() override;
    void Draw() override;
    //敵にヒットした時の処理
    void OnHitEnemy(ColliderTag hitTag) override;
    //ダメージが減らされるか
    bool IsDamageReduced() const override { return m_isDamageReduced; }

private:
    //ホーミング先のターゲット
    std::weak_ptr<EnemyBase> m_pTarget;
    //ホーミングの強さ
    float m_homingStrength = 0.0f;
    //受け取った弾の速度
    float m_speed = 0.0f;
    //ダメージが減らされるか
    bool m_isDamageReduced = true;
    //Effekseerエフェクト再生ハンドル
	int m_effectPlayH = -1;
	//ロケットのモデル
	std::shared_ptr<RocketModel> m_pModel = nullptr;
};
