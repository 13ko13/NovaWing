#pragma once
#include "Scene.h"
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

	//現在プレイヤーが選んでいる選択肢
	TitleSelect m_selectIndex = TitleSelect::StartGame;
};

