#pragma once
#include "GaugeUIBase.h"

class SpiderBoss;
class BossHPGaugeUI : public GaugeUIBase
{
public:
    //ボスからHPを貰うためボスの弱参照を受け取る
    BossHPGaugeUI(std::weak_ptr<SpiderBoss> pBoss);
    ~BossHPGaugeUI();

    //描画
    void Draw() override;
    //更新
    void Update() override;

private:
    std::weak_ptr<SpiderBoss> m_pBoss;
};

