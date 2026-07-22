#pragma once
#include <memory>

#include "UIBase.h"

class Player;
class HPGaugeUI : public UIBase
{
public:
    //プレイヤーからHPを貰うためプレイヤーの弱参照を受け取る
    HPGaugeUI(std::weak_ptr<Player> pPlayer);
    //描画
    void Draw() override;
    //更新
    void Update() override;

private:
    //時間
    int m_frame = 0;

    //プレイヤーの弱参照
    std::weak_ptr<Player> m_pPlayer;

    //UIにかけるグリッチシェーダのハンドル
    int m_glitchPSH = -1;

    //グリッチシェーダに渡すためのシェーダバッファ
    struct GlitchBuffer
    {
        float time;
        float dummy[3];//16バイトアライメント
    };
    int m_cbufferGlitch = -1;
    GlitchBuffer* m_pCBuffGlitchData = nullptr;
};

