#pragma once

#include "Scene.h"

class TutorialMessageScene : public Scene
{
public:
    TutorialMessageScene(SceneController& controller);
	virtual ~TutorialMessageScene();

	//初期化
	void Init() override;
	//更新
	void Update() override;
	//描画
	void Draw() override;
	//自身のシーンIDを返す
	SceneID GetSceneID() const override;

private:
	//テキストのフォントハンドル
	int m_textFontHandle = -1;
};