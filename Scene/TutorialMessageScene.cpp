#include <DxLib.h>

#include "TutorialMessageScene.h"
#include "Main/Application.h"
#include "Manager/ResourceLoader.h"
#include "Manager/InputManager.h"
#include "SceneController.h"
#include "Utility/Vector2.h"

namespace
{
	constexpr float message_box_start_x_ratio = 0.2f;
	constexpr float message_box_end_x_ratio = 0.8f;
	constexpr float message_box_start_y_ratio = 0.8f;
	constexpr float message_box_end_y_ratio = 0.95f;

	//メッセージを連打ですぐに閉じてしまうのを防止する
	//閉じられるようになるまでのフレーム
	constexpr int close_enable_frame = 60;

	//「Aで次へ」の画像の位置(メッセージの箱の右下の角から、画像の中心までのずらし量。1920x1080基準)
	//リザルト画面と同じ並び(Aボタンの右に「次へ」、中心同士の間隔60)にしている
	const Vector2 a_button_offset = Vector2(-110.0f, -45.0f);
	const Vector2 next_text_offset = Vector2(-50.0f, -45.0f);
	//「Aで次へ」の画像の拡大率(リザルト画面と同じ)
	constexpr double next_prompt_scale = 0.45;
}

TutorialMessageScene::TutorialMessageScene(
	SceneController& controller,
	const std::wstring& hintText) :
	Scene(controller),
	m_hintText(hintText)
{
}

TutorialMessageScene::~TutorialMessageScene()
{
}

void TutorialMessageScene::Init()
{
	m_textFontHandle = ResourceLoader::GetInstance().GetFont(
		ResourceLoader::FontID::Message
	);
}

void TutorialMessageScene::Update()
{
	m_frame++;

	InputManager& input = InputManager::GetInstance();
	//OKボタンが押されたら閉じる(PopScene)
	if (input.IsTriggered(InputEvent::ok) &&
		CanClose())
	{
		m_controller.PopScene();
	}
}

void TutorialMessageScene::Draw()
{
	//画面サイズ
	const auto& wsize = Application::GetInstance().GetWindowSize();

	//ボックスの描画位置
	int x1;
	int x2;
	int y1;
	int y2;
	x1 = static_cast<int>(wsize.width * message_box_start_x_ratio);
	x2 = static_cast<int>(wsize.width * message_box_end_x_ratio);
	y1 = static_cast<int>(wsize.height * message_box_start_y_ratio);
	y2 = static_cast<int>(wsize.height * message_box_end_y_ratio);

	//黒いボックスの上にメッセージを表示
	DrawBox(x1, y1, x2, y2, 0x000000, true);
	DrawStringToHandle(x1, y1, m_hintText.c_str(), 0xffffff, m_textFontHandle);

	//Aボタンの画像
	int buttonAGraphH = ResourceLoader::GetInstance().GetGraphic(
		ResourceLoader::GraphicID::ButtonA);
	//次へテキストの画像
	int nextTextGraphH = ResourceLoader::GetInstance().GetGraphic(
		ResourceLoader::GraphicID::NextText
	);

	float uiScale = Application::GetInstance().GetUIScale();

	if (CanClose())
	{
		//箱の右下の角を基準に、ずらし量(UIスケールを掛ける)だけ内側に描く
		DrawRotaGraph(
			static_cast<int>(x2 + a_button_offset.x * uiScale),
			static_cast<int>(y2 + a_button_offset.y * uiScale),
			next_prompt_scale * uiScale, 0.0, buttonAGraphH, true);
		DrawRotaGraph(
			static_cast<int>(x2 + next_text_offset.x * uiScale),
			static_cast<int>(y2 + next_text_offset.y * uiScale),
			next_prompt_scale * uiScale, 0.0, nextTextGraphH, true);
	}
}

SceneID TutorialMessageScene::GetSceneID() const
{
	//ゲーム中に出るシーンなのでゲームとする
	return SceneID::Game;
}

bool TutorialMessageScene::CanClose() const
{
	return m_frame >= close_enable_frame;
}
