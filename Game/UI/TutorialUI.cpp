#include <DxLib.h>

#include "TutorialUI.h"
#include "Utility/Vector2.h"
#include "Main/Application.h"

namespace
{
	//タスクの背景を表示する割合位置
	const Vector2 task_box_pos_ratio = Vector2(0.87f, 0.1f);
	//タスクの背景の中心から横の端までの長さ
	constexpr float center_to_horizontal_end = 150.0f;
	//タスクの背景の中心から縦の端までの長さ
	constexpr float center_to_vertical_end = 20.0f;
}

TutorialUI::TutorialUI()
{
}

TutorialUI::~TutorialUI()
{
}

void TutorialUI::Draw()
{
	switch (m_taskDrawState)
	{
	case TaskDrawState::Hide:
		break;

	case TaskDrawState::Show:
	{
		//ウィンドウサイズ
		const auto& wsize = Application::GetInstance().GetWindowSize();
		Vector2 taskBoxPos = Vector2(
			wsize.width * task_box_pos_ratio.x, wsize.height * task_box_pos_ratio.y
		);

		//DrawBoxで枠を作っておく
		DrawBox(
			static_cast<int>(taskBoxPos.x - center_to_horizontal_end),
			static_cast<int>(taskBoxPos.y - center_to_vertical_end),
			static_cast<int>(taskBoxPos.x + center_to_horizontal_end),
			static_cast<int>(taskBoxPos.y + center_to_vertical_end),
			0x000000, true
		);
	}
		break;

	case TaskDrawState::Success:
		break;
	}
}

void TutorialUI::Update()
{
}

void TutorialUI::OnStart(const std::wstring& taskText)
{
	//文字を受け取る
	m_taskText = taskText;
	//表示を開始する
	m_taskDrawState = TaskDrawState::Show;
}

void TutorialUI::OnSuccess()
{
	//ステートを成功に変更
	m_taskDrawState = TaskDrawState::Success;
}

void TutorialUI::OnEnd()
{
	//ステートを非表示に戻す
	m_taskDrawState = TaskDrawState::Hide;
}
