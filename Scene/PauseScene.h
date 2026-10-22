#pragma once
#include <array>
#include <memory>
#include <functional>

#include "Scene.h"

class SoundManager;

class PauseScene : public Scene
{
public:
	//pSoundManagerはGameSceneから渡す
	//(ポーズ中も鳴り続けさせるため、PauseScene破棄の影響を受けないようweak_ptrで借りる)
	PauseScene(
		SceneController& controller,
		std::weak_ptr<SoundManager> pSoundManager);
	~PauseScene();

	void Init() override;
	void Update() override;
	void Draw() override;
	SceneID GetSceneID() const override;//自身のシーンIDを返す

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

	//サウンドマネージャーへのポインタ(GameSceneが所有するものを借りている)
	std::weak_ptr<SoundManager> m_pSoundManager;
};