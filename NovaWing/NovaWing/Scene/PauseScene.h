#pragma once
#include <array>

#include "Scene.h"

class PauseScene : public Scene
{
public:
	PauseScene(SceneController& controller);
	~PauseScene();

	void Init() override;
	void Update() override;
	void Draw() override;

private:
	//選択肢
	enum class Select
	{
		BackGame,//ゲームに戻る
		BackTitle,//タイトルに戻る

		Max,//最大
	};
	Select m_select = Select::BackGame;//現在の選択肢

	Select m_prevSelectIdx = Select::BackGame;
	std::array<float, static_cast<size_t>(Select::Max)> m_wipeProgress = {};
	int m_glitchPSH = -1;
	struct GlitchBuffer
	{
		float time;
		float scanlineFrequency;
		float dummy[2];
	};
	int m_cbufferGlitch = -1;
	GlitchBuffer* m_pCBuffGlitchData = nullptr;
	int m_frame = 0;
	int m_backGroundOpenFrame = 0;
};