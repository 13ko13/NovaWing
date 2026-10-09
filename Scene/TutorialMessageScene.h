#pragma once

#include "Scene.h"
#include <string>

class TutorialMessageScene : public Scene
{
public:
	TutorialMessageScene(SceneController& controller,
		const std::wstring& hintText);
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
	//ヒントテキスト
	std::wstring m_hintText;
};