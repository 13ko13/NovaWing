#pragma once
#include "Scene.h"
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
};