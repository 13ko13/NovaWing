#include "ClearScene.h"
#include "Manager/InputManager.h"
#include "Scene/TitleScene.h"
#include "SceneController.h"
#include "Main/Application.h"

namespace
{
	//決定キーの遷移先に切り替わるまでのフレーム数
	constexpr float scene_change_frame = 60.0f;

	//選択肢のカーソル
	constexpr const wchar_t* cursor_text = L"→";
	//クリア表示のテキスト
	constexpr const wchar_t* game_clear_text = L"GAME CLEAR";
	//選択肢のテキスト
	constexpr const wchar_t* back_title_text = L"タイトルに戻る";
	constexpr const wchar_t* exit_game_text = L"ゲーム終了";
}

ClearScene::ClearScene(SceneController& controller):
    Scene(controller)
{
}

ClearScene::~ClearScene()
{
}

void ClearScene::Init()
{
}

void ClearScene::Update()
{
    //InputManagerのインスタンスを取得
	InputManager& input = InputManager::GetInstance();

	//下入力で選択肢を下に移動(indexを増やす) 
	if (input.IsTriggered(InputEvent::down))
	{
		//選択肢の最大数で割った余りを取ることで、
		//選択肢の範囲内に収める
		m_selectIndex =
			static_cast<ClearSelect>(
				(static_cast<int>(m_selectIndex) + 1) %
				static_cast<int>(ClearSelect::SelectMax));
	}
	//上入力で選択肢を上に移動(indexを減らす)
	if (input.IsTriggered(InputEvent::up))
	{
		//選択肢の最大数で割った余りを取ることで、
		//選択肢の範囲内に収める
		m_selectIndex =
			static_cast<ClearSelect>(
				(static_cast<int>(m_selectIndex) - 1 + static_cast<int>(ClearSelect::SelectMax)) %
				static_cast<int>(ClearSelect::SelectMax));
	}
	//決定入力で選択肢を決定する
	if (input.IsTriggered(InputEvent::ok))
	{
		switch (m_selectIndex)
		{
		case ClearSelect::BackTitle:
		{
			//タイトルシーンに遷移する
			m_controller.ChangeScene(
				std::make_shared<TitleScene>(
					m_controller), scene_change_frame);
			break;
		}
		case ClearSelect::ExitGame:
		{
			//ゲームを終了する
			Application::GetInstance().RequestExit();
			break;
		}
		}
	}
}

void ClearScene::Draw()
{
    //ウィンドウサイズ
	Size wsize = Application::GetInstance().GetWindowSize();
	//画面の真ん中
	int x = wsize.m_width / 2;
	int y = wsize.m_height / 2;
    DrawFormatString(x, y, 0xffff00, game_clear_text);

	//とりあえず左上に選択肢を表示する
	//選択中の選択肢に矢印を表示する
	switch (m_selectIndex)
	{
	case ClearSelect::BackTitle:
	{
		DrawFormatString(0, 15, 0xffffff, cursor_text);
		DrawFormatString(15, 15, 0xff0000, back_title_text);
		DrawFormatString(15, 30, 0xffffff, exit_game_text);
		break;
	}
	case ClearSelect::ExitGame:
	{
		DrawFormatString(0, 30, 0xffffff, cursor_text);
		DrawFormatString(15, 15, 0xffffff, back_title_text);
		DrawFormatString(15, 30, 0xff0000, exit_game_text);
		break;
	}
	}
}
