#pragma once
#include "Scene.h"

class ClearScene : public Scene
{
public:
    ClearScene(SceneController& controller);
	~ClearScene();

    void Init() override;
	void Update() override;
	void Draw() override;

private:
	//クリアシーンの選択肢
	enum class ClearSelect
	{
		BackTitle,//タイトルに戻る
		ExitGame,//ゲーム終了

		SelectMax,//選択肢の最大数
	};
	//現在プレイヤーが選んでいる選択肢
	ClearSelect m_selectIndex = ClearSelect::BackTitle;
};

