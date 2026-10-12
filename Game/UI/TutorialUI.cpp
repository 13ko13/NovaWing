#define NOMINMAX
#include <DxLib.h>
#include <algorithm>

#include "TutorialUI.h"
#include "Utility/Vector2.h"
#include "Main/Application.h"
#include "Manager/ResourceLoader.h"

namespace
{
	//タスクの背景を表示する割合位置
	const Vector2 task_box_pos_ratio = Vector2(0.87f, 0.1f);
	//タスクの背景の中心から横の端までの長さ
	constexpr float center_to_horizontal_end = 150.0f;
	//タスクの背景の中心から縦の端までの長さ
	constexpr float center_to_vertical_end = 20.0f;
	//タスクの背景の枠の線の太さ
	constexpr int task_frame_line_thickness = 3;
	//チェックマーク用のボックスの端までの長さ
	constexpr float check_box_center_to_end = 13.0f;
	//チェックボックスを表示する割合位置
	const Vector2 check_box_pos_ratio = Vector2(0.805f, 0.1f);
	//チェックマークの各点の、チェックボックス中心からのずらし量
	const Vector2 check_mark_start_offset = Vector2(-8.0f, 0.0f);	//左の始点
	const Vector2 check_mark_corner_offset = Vector2(-3.0f, 6.0f);  //下の角
	const Vector2 check_mark_end_offset = Vector2(8.0f, -7.0f);  //右上の終点
	//チェックマークのアニメーションにかける時間
	constexpr int check_anim_max_frame = 20;
	//チェックマークの1本目と2本目の線の境目(この割合までに1本目を書く)
	constexpr float check_first_line_goal_rate = 0.35f;
	//チェックマークの線の太さ
	constexpr int check_thickness = 3;
}

TutorialUI::TutorialUI()
{
	//タスク用のフォントハンドルを取得
	m_taskFontHandle = ResourceLoader::GetInstance().GetFont(ResourceLoader::FontID::Task);
}

TutorialUI::~TutorialUI()
{
}

void TutorialUI::Draw()
{
	//非表示ならスキップ
	if (m_taskDrawState == TaskDrawState::Hide) return;

	//枠と文字の描画
	Application& app = Application::GetInstance();
	//ウィンドウサイズ
	const auto& wsize = app.GetWindowSize();
	Vector2 taskBoxPos = Vector2(
		wsize.width * task_box_pos_ratio.x, wsize.height * task_box_pos_ratio.y
	);
	Vector2 checkBoxPos = Vector2(
		wsize.width * check_box_pos_ratio.x, wsize.height * check_box_pos_ratio.y
	);

	//ウィンドウサイズに合わせたUIのスケール
	float uiScale = app.GetUIScale();

	//DrawBoxで背景を作っておく
	DrawBox(
		static_cast<int>(taskBoxPos.x - center_to_horizontal_end * uiScale),
		static_cast<int>(taskBoxPos.y - center_to_vertical_end * uiScale),
		static_cast<int>(taskBoxPos.x + center_to_horizontal_end * uiScale),
		static_cast<int>(taskBoxPos.y + center_to_vertical_end * uiScale),
		0x777777, true
	);

	//背景の枠の色
	unsigned int taskBoxFrameCol = 0x00bb00;

	if (m_taskDrawState == TaskDrawState::Success)
	{
		//成功時色を変える
		taskBoxFrameCol = 0xdddd00;
	}
	//背景の枠
	DrawBox(
		static_cast<int>(taskBoxPos.x - center_to_horizontal_end * uiScale),
		static_cast<int>(taskBoxPos.y - center_to_vertical_end * uiScale),
		static_cast<int>(taskBoxPos.x + center_to_horizontal_end * uiScale),
		static_cast<int>(taskBoxPos.y + center_to_vertical_end * uiScale),
		taskBoxFrameCol, false, task_frame_line_thickness
	);

	//テキスト描画
	DrawStringToHandle(
		static_cast<int>(taskBoxPos.x - 100 * uiScale),
		static_cast<int>(taskBoxPos.y - center_to_vertical_end * uiScale),
		m_taskText.c_str(), 0xffffff, m_taskFontHandle
	);

	//チェックボックスの描画
	DrawBox(
		static_cast<int>(checkBoxPos.x - check_box_center_to_end * uiScale),
		static_cast<int>(checkBoxPos.y - check_box_center_to_end * uiScale),
		static_cast<int>(checkBoxPos.x + check_box_center_to_end * uiScale),
		static_cast<int>(checkBoxPos.y + check_box_center_to_end * uiScale),
		0x000000, true
	);

	if (m_taskDrawState == TaskDrawState::Success)
	{
		DrawCheckMark(checkBoxPos);
	}
}

void TutorialUI::DrawCheckMark(const Vector2& checkBoxPos)
{
	//ウィンドウサイズに合わせたUIのスケール
	float uiScale = Application::GetInstance().GetUIScale();

	//成功したときだけ追加の演出を行う
	float progress = static_cast<float>(m_checkAnimFrame) / static_cast<float>(check_anim_max_frame);
	progress = std::min(progress, 1.0f);

	//1本目の線
	float firstLineProgress = progress / check_first_line_goal_rate;
	firstLineProgress = std::min(firstLineProgress, 1.0f);
	Vector2 currentLinePos = Vector2::Lerp(
		checkBoxPos + check_mark_start_offset * uiScale,
		checkBoxPos + check_mark_corner_offset * uiScale,
		firstLineProgress
	);

	//1本目の線の描画
	DrawLineAA(
		checkBoxPos.x + check_mark_start_offset.x * uiScale,
		checkBoxPos.y + check_mark_start_offset.y * uiScale,
		currentLinePos.x,
		currentLinePos.y,
		0xffffff, check_thickness * uiScale
	);

	//2本目の線
	//1本目の進捗を除いた進捗を計算
	float secondLineProgress =
		(progress - check_first_line_goal_rate) /
		(1.0f - check_first_line_goal_rate);
	//1本目が書き終わっていたら描画
	if (progress > check_first_line_goal_rate)
	{
		Vector2 secondCurrentLinePos = Vector2::Lerp(
			checkBoxPos + check_mark_corner_offset * uiScale,
			checkBoxPos + check_mark_end_offset * uiScale,
			secondLineProgress
		);

		DrawLineAA(
			checkBoxPos.x + check_mark_corner_offset.x * uiScale,
			checkBoxPos.y + check_mark_corner_offset.y * uiScale,
			secondCurrentLinePos.x,
			secondCurrentLinePos.y,
			0xffffff, check_thickness * uiScale
		);
	}
}

void TutorialUI::Update()
{
	if (m_taskDrawState == TaskDrawState::Success)
	{
		m_checkAnimFrame++;
	}
}

void TutorialUI::OnStart(const std::wstring& taskText)
{
	if (m_taskDrawState != TaskDrawState::Hide) return;

	//文字を受け取る
	m_taskText = taskText;
	//表示を開始する
	m_taskDrawState = TaskDrawState::Show;
}

void TutorialUI::OnSuccess()
{
	//ステートを成功に変更
	m_taskDrawState = TaskDrawState::Success;
	//アニメーションのフレームリセット
	m_checkAnimFrame = 0;
}

void TutorialUI::OnEnd()
{
	//ステートを非表示に戻す
	m_taskDrawState = TaskDrawState::Hide;
}
