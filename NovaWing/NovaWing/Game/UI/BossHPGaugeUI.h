#pragma once
#include "GaugeUIBase.h"

class BossEnemy;
class BossHPGaugeUI : public GaugeUIBase
{
public:
    //ボスからHPを貰うためボスの弱参照を受け取る
    BossHPGaugeUI(std::weak_ptr<BossEnemy> pBoss);
    ~BossHPGaugeUI();

    //描画
    void Draw() override;
    //更新
    void Update() override;

private:
    std::weak_ptr<BossEnemy> m_pBoss;
};

