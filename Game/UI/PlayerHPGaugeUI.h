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
    //更新
    void Update() override;

private:
#ifdef _DEBUG
    float m_scanlineFrequency = 0.0f;
#endif
	//プレイヤーの弱参照
	std::weak_ptr<Player> m_pPlayer;
};

