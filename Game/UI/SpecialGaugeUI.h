#pragma once
#include "GaugeUIBase.h"

class Player;
class SpecialGaugeUI : public GaugeUIBase
{
public:
    //ボスからHPを貰うためプレイヤーの弱参照を受け取る
    SpecialGaugeUI(std::weak_ptr<Player> pPlayer);
    ~SpecialGaugeUI();

    //描画
    void Draw() override;
    //更新
    void Update() override;

private:
    //プレイヤーの弱参照
    std::weak_ptr<Player> m_pPlayer;
};

