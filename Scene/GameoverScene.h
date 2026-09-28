#pragma once
#include <array>
#include <memory>

#include "Scene.h"

class SoundManager;
class GameoverScene : public Scene
{
public:
	GameoverScene(SceneController& controller);
	~GameoverScene();

	void Init() override;
	void Update() override;
	void Draw() override;

private:
	//ゲームオーバーシーンの選択肢
	enum class GameoverSelect
	{
		Retry,//リトライ
		ExitGame,//ゲーム終了

		SelectMax,//選択肢の最大数
	};
	//現在プレイヤーが選んでいる選択肢
	GameoverSelect m_selectIndex = GameoverSelect::Retry;
	GameoverSelect m_prevSelectIdx = GameoverSelect::Retry;//前のフレーム

	//各選択に対応するワイプの進行度
	std::array<float, static_cast<size_t>(GameoverSelect::SelectMax)> m_wipeProgress = {};

	//グリッチシェーダのハンドル
	int m_glitchPSH = -1;

	//グリッチシェーダに渡すためのシェーダバッファ
	struct GlitchBuffer
	{
		float time;
		float scanlineFrequency;
		float dummy[2];//16バイトアライメント
	};
	int m_cbufferGlitch = -1;
	GlitchBuffer* m_pCBuffGlitchData = nullptr;

	//フレーム計測（時間をシェーダに渡すために必要）
	int m_frame = 0;

	//選択肢背景のカーテン演出のためのフレーム
	int m_backGroundOpenFrame = 0;

	//サウンドマネージャー
	std::shared_ptr<SoundManager> m_pSoundManager;
};