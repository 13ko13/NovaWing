#pragma once
#include <memory>

#include "UIBase.h"

class Player;
struct SizeF;
class HPGaugeUI : public UIBase
{
public:
    //プレイヤーからHPを貰うためプレイヤーの弱参照を受け取る
    HPGaugeUI(std::weak_ptr<Player> pPlayer);
    //描画
    void Draw() override;

private:
    //矩形1枚をシェーダ付きで描画する
    void DrawGraphToShader(
        float left, float top,
        const SizeF& size, float uvMaxU, int texH
    );

private:
    //プレイヤーの弱参照
    std::weak_ptr<Player> m_pPlayer;
    //UIにかけるグリッチシェーダのハンドル
    int m_glitchPSH = -1;
};

