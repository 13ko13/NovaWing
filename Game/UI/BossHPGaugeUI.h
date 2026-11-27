#pragma once
#include "GaugeUIBase.h"

class BossBase;
class BossHPGaugeUI : public GaugeUIBase
{
public:
    //ボスからHPを貰うためボスの弱参照を受け取る
    BossHPGaugeUI(std::weak_ptr<BossBase> pBoss);
    ~BossHPGaugeUI();

    //描画
    void Draw() override;
    //更新
    void Update() override;

private:
	std::weak_ptr<BossBase> m_pBoss;

	//前のフレームの合計ダメージ
	float m_prevTotalHitDamage = 0.0f;
	//揺れの残りフレーム数
	int m_shakeFrame = 0;
};

