#pragma once
#include <memory>

#include "GaugeUIBase.h"

class Player;
class PlayerHPGaugeUI : public GaugeUIBase
{
public:
    //プレイヤーからHPを貰うためプレイヤーの弱参照を受け取る
    PlayerHPGaugeUI(std::weak_ptr<Player> pPlayer);
    ~PlayerHPGaugeUI();

    //描画
    void Draw() override;

private:
	//プレイヤーの弱参照
	std::weak_ptr<Player> m_pPlayer;
};

