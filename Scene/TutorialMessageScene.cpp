#include <DxLib.h>

#include "TutorialMessageScene.h"
#include "Main/Application.h"
#include "Manager/ResourceLoader.h"

namespace
{
	constexpr float message_box_start_x_ratio = 0.2f;
	constexpr float message_box_end_x_ratio = 0.8f;
	constexpr float message_box_start_y_ratio = 0.8f;
	constexpr float message_box_end_y_ratio = 0.95f;
}

TutorialMessageScene::TutorialMessageScene(SceneController& controller) :
	Scene(controller)
{
}

TutorialMessageScene::~TutorialMessageScene() {
}

void TutorialMessageScene::Init()
{
	
}

void TutorialMessageScene::Update()
{
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
	DrawBox(x1, y1, x2, y2, 0xffffff, true);
	DrawString(x1,y1, L"テストメッセージ", 0x000000);

#ifdef _DEBUG
	DrawFormatString(0, 15, 0xffffff, L"TutorialMessageScene");
#endif
}

SceneID TutorialMessageScene::GetSceneID() const
{
	return SceneID();
}
