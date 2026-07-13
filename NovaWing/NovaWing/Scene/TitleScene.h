#pragma once
#include <array>

#include "Scene.h"
#include "Manager/ResourceLoader.h"

class TitleScene : public Scene
{
public:
	TitleScene(SceneController& controller);
	~TitleScene();

	void Init() override;//初期化処理
	void Update() override;//更新処理
	void Draw() override;//描画処理

private:
	enum class TitleSelect
	{
		StartGame,//ゲーム開始
		ExitGame,//ゲーム終了

		SelectMax,//選択肢の最大数
	};

	//各選択に対応するワイプの進行度
	std::array<float, static_cast<size_t>(TitleSelect::SelectMax)> m_wipeProgress = {};

	//現在プレイヤーが選んでいる選択肢
	TitleSelect m_selectIndex = TitleSelect::StartGame;
	TitleSelect m_prevSelectIdx = TitleSelect::StartGame;//前のフレーム

	//フレーム計測
	int m_frame = 0;

	//タイトルロゴ
	int m_titleLogoH = -1;
	//ゲーム開始画像ハンドル
	int m_gameStartGraphH = -1;
	//ゲーム終了画像ハンドル
	int m_gameEndGraphH = -1;
	//カーソルが乗っているときのゲーム開始画像ハンドル
	int m_gameStartOnCursorGraphH = -1;
	//カーソルが乗っているときのゲーム終了画像ハンドル
	int m_gameEndOnCursorGraphH = -1;
	//選択肢の背景画像
	int m_selectBackGroundH = -1;
};

