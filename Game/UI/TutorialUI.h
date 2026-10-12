#pragma once
#include <string>

#include "UIBase.h"

struct Vector2;
class TutorialUI : public UIBase
{
public:
	TutorialUI();
	virtual ~TutorialUI();
	void Draw() override;
	void Update() override;
	/// 表示を開始する
	void OnStart(const std::wstring& taskText);
	/// タスクを成功
	void OnSuccess();
	/// 表示終了
	void OnEnd();

private:
	void DrawCheckMark(const Vector2& checkBoxPos);

private:
	std::wstring m_taskText;
	//今の状態
	enum class TaskDrawState
	{
		Hide,//非表示
		Show,//表示
		Success,//成功
	};
	TaskDrawState m_taskDrawState = TaskDrawState::Hide;

	//タスク表示用のフォントハンドル
	int m_taskFontHandle = -1;

	//チェックマークのアニメーション用フレーム
	int m_checkAnimFrame = 0;
};
