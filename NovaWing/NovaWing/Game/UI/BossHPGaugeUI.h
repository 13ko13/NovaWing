#pragma once
#include "HPGaugeUIBase.h"

class BossEnemy;
class BossHPGaugeUI : public HPGaugeUIBase
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
};

