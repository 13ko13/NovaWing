#pragma once
#include <memory>

#include "UIBase.h"

class TargetManager;
class Player;
class ReticleUI : public UIBase
{
public:
	ReticleUI(std::weak_ptr<TargetManager> pTargetManager,
	std::weak_ptr<Player> pPlayer);
	~ReticleUI();

	//描画処理
	void Draw() override;
	//更新処理
	void Update() override;

private:
	//プレイヤー
	std::weak_ptr<Player> m_pPlayer;

	//ターゲットマネージャー
	std::weak_ptr<TargetManager> m_pTargetManager;
	//フォーカス時のアルファ用
	float m_focusAlpha = 0.0f;

	//チャージレティクルアニメーション進行度
	float m_animProgress = 0.0f;
};

