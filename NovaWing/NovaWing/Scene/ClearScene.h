#pragma once
#include <array>
#include <memory>

#include "Scene.h"

class SoundManager;
class ClearScene : public Scene
{
public:
	//クリア時のリザルトとして必要な情報
	struct ClearResultData
	{
		int defeatedEnemyCount;
		int clearTime;
		int hitCount;
	};

    ClearScene(SceneController& controller,const ClearResultData& data);
	~ClearScene();

    void Init() override;
	void Update() override;
	void Draw() override;

private:
	//現在のLerp値でテキストオフスクリーンを書き直す
	void DrawResultText(int fontHandle);

private:
	//クリアシーンの選択肢
	enum class ClearSelect
	{
		ReTry,//リトライ
		BackTitle,//タイトルに戻る

		SelectMax,//選択肢の最大数
	};
	//現在プレイヤーが選んでいる選択肢
	ClearSelect m_selectIndex = ClearSelect::ReTry;
	ClearSelect m_prevSelectIdx = ClearSelect::ReTry;//前のフレーム

	//各選択に対応するワイプの進行度
	std::array<float, static_cast<size_t>(ClearSelect::SelectMax)> m_wipeProgress = {};

	//リザルトとして表示する情報
	ClearResultData m_resultData;

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

	//テンプレート画像のカーテン演出のためのフレーム
	int m_templeteOpenFrame = 0;
	//背景のカーテン演出のためのフレーム
	int m_backGroundOpenFrame = 0;

	//文字にシェーダをかけるためのレンダーターゲットハンドル
	int m_textRenderTargetH = -1;

	//GraphFilterでぼかした発光用画像
	int m_textGlowH = -1;

	//数字をLerpで表示するための、現在の表示値
	float m_currentKillCount = 0.0f;
	float m_currentClearTime = 0.0f;
	float m_currentHitCount = 0.0f;
	float m_currentScore = 0.0f;
	//lerpが終了したか
	bool m_isLerpFinished = false;

	//スコア
	int m_score = 0;

	//次へボタンが押されたか
	bool m_isPushNextButton = false;

	//スコア加算音のクールタイム計測
	int m_scoreCountSoundCT = 0;

	//サウンドマネージャー
	std::shared_ptr<SoundManager> m_pSoundManager;
};