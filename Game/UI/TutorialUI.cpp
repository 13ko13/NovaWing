#define NOMINMAX
#include <DxLib.h>
#include <algorithm>

#include "TutorialUI.h"
#include "Utility/Vector2.h"
#include "Main/Application.h"
#include "Manager/ResourceLoader.h"

namespace
{
	//タスクの背景の右端の割合位置
	constexpr float task_box_right_ratio = 0.96f;
	//タスクの背景の割合位置Y
	constexpr float task_box_ratio_y = 0.1f;
	//タスクの背景の中心から縦の端までの長さ
	constexpr float center_to_vertical_end = 20.0f;
	//タスクの背景の枠の線の太さ
	constexpr int task_frame_line_thickness = 3;
	//タスクの背景の枠の色(Aボタン・「次へ」の画像と同じ緑)
	constexpr unsigned int task_frame_color = 0x60e9a4;
	//タスク成功時の枠の色
	constexpr unsigned int task_frame_success_color = 0xdddd00;
	//チェックマーク用のボックスの端までの長さ
	constexpr float check_box_center_to_end = 13.0f;
	//タスクの背景の端と文字の間の余白
	constexpr float task_box_margin = 15.0f;
	//チェックボックスと文字の隙間
	constexpr float check_to_text_space = 10.0f;
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
	//スライド演出にかけるフレーム数
	constexpr int slide_anim_max_frame = 10;
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

	Application& app = Application::GetInstance();
	//ウィンドウサイズ
	const auto& wsize = app.GetWindowSize();
	
	//ウィンドウサイズに合わせたUIのスケール
	float uiScale = app.GetUIScale();

	//スライド演出の進行度
	float slideAnimProgress = static_cast<float>(m_slideAnimFrame) /
		static_cast<float>(slide_anim_max_frame);
	//上限を決めておく
	slideAnimProgress = std::min(slideAnimProgress, 1.0f);
	//スライドアウトの際は逆にしておく
	if (m_taskDrawState == TaskDrawState::SlideOut)
	{
		slideAnimProgress = 1.0f - slideAnimProgress;
	}

	//枠と文字の描画
	//背景の横幅
	//文字と横の端との余白、チェックボックスの幅を考慮して、横幅を決める
	float boxWidth = (task_box_margin * 2 + check_box_center_to_end * 2 +
		 check_to_text_space) * uiScale + m_textWidth;

	slideAnimProgress = 1.0f - (1.0f - slideAnimProgress) * (1.0f - slideAnimProgress);

	//画面外の右端
	float screenOutRight = static_cast<float>(wsize.width) + boxWidth;

	//DrawBoxで背景を作っておく
	//文字列の長さに合わせて大きさを変えたいので先に枠の右端の位置を決める
	//目標の枠右端位置
	float boxRightTarget = wsize.width * task_box_right_ratio;
	//現在の枠右端位置
	float currentBoxRight = screenOutRight + (boxRightTarget - screenOutRight) * slideAnimProgress;
	//チェックボックスの大きさと、枠の余白を考えて左端を決める
	float boxLeft = currentBoxRight - boxWidth;
	
	DrawBox(
		static_cast<int>(boxLeft),
		static_cast<int>(wsize.height * task_box_ratio_y - center_to_vertical_end * uiScale),
		static_cast<int>(currentBoxRight),
		static_cast<int>(wsize.height * task_box_ratio_y + center_to_vertical_end * uiScale),
		0x777777, true
	);

	//背景の枠の色
	unsigned int taskBoxFrameCol = task_frame_color;

	if (m_taskDrawState == TaskDrawState::Success ||
		m_taskDrawState == TaskDrawState::SlideOut)
	{
		//成功時色を変える
		taskBoxFrameCol = task_frame_success_color;
	}
	//背景の枠
	DrawBox(
		static_cast<int>(boxLeft),
		static_cast<int>(wsize.height * task_box_ratio_y - center_to_vertical_end * uiScale),
		static_cast<int>(currentBoxRight),
		static_cast<int>(wsize.height * task_box_ratio_y + center_to_vertical_end * uiScale),
		taskBoxFrameCol, false, task_frame_line_thickness
	);

	//チェックボックスのX座標は大枠からチェックボックスまでの
	//余白とチェックボックスの中心から端までを足したもの
	float checkBoxX = boxLeft + (task_box_margin + check_box_center_to_end) * uiScale;
	Vector2 checkBoxPos = Vector2(checkBoxX, wsize.height * task_box_ratio_y);

	//テキスト描画
	float textLeft = checkBoxPos.x + (check_box_center_to_end + check_to_text_space) * uiScale;
	DrawStringToHandle(
		static_cast<int>(textLeft),
		static_cast<int>(checkBoxPos.y - GetFontSizeToHandle(m_taskFontHandle) / 2),
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

	if (m_taskDrawState == TaskDrawState::Success ||
		m_taskDrawState == TaskDrawState::SlideOut)
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

	//右から出てくるような演出
	if (m_taskDrawState == TaskDrawState::SlideIn ||
		m_taskDrawState == TaskDrawState::SlideOut)
	{
		m_slideAnimFrame++;
	}
	if(m_slideAnimFrame > slide_anim_max_frame)
	{
		if (m_taskDrawState == TaskDrawState::SlideIn) m_taskDrawState = TaskDrawState::Show;
		else if (m_taskDrawState == TaskDrawState::SlideOut) m_taskDrawState = TaskDrawState::Hide;
	}
}

void TutorialUI::OnStart(const std::wstring& taskText)
{
	if (m_taskDrawState != TaskDrawState::Hide) return;

	m_slideAnimFrame = 0;

	//文字を受け取る
	m_taskText = taskText;
	//表示を開始する
	m_taskDrawState = TaskDrawState::SlideIn;
	//文字の幅を測る
	m_textWidth = GetDrawStringWidthToHandle(
		m_taskText.c_str(), static_cast<int>(m_taskText.size()), m_taskFontHandle);
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
	m_taskDrawState = TaskDrawState::SlideOut;
	m_slideAnimFrame = 0;
}
