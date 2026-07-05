#include "GameoverScene.h"
#include "Manager/InputManager.h"
#include "SceneController.h"
#include "GameScene.h"
#include "Main/Application.h"

GameoverScene::GameoverScene(SceneController& controller):
	Scene(controller)
{
}

GameoverScene::~GameoverScene()
{
}

void GameoverScene::Init()
{
}

void GameoverScene::Update()
{
	//InputManagerのインスタンスを取得
	InputManager& input = InputManager::GetInstance();

	//下入力で選択肢を下に移動(indexを増やす) 
	if (input.IsTriggered("down"))
	{
		//選択肢の最大数で割った余りを取ることで、
		//選択肢の範囲内に収める
		m_selectIndex =
			static_cast<GameoverSelect>(
				(static_cast<int>(m_selectIndex) + 1) %
				static_cast<int>(GameoverSelect::SelectMax));
	}
	//上入力で選択肢を上に移動(indexを減らす)
	if (input.IsTriggered("up"))
	{
		//選択肢の最大数で割った余りを取ることで、
		//選択肢の範囲内に収める
		m_selectIndex =
			static_cast<GameoverSelect>(
				(static_cast<int>(m_selectIndex) - 1 + static_cast<int>(GameoverSelect::SelectMax)) %
				static_cast<int>(GameoverSelect::SelectMax));
	}
	//決定入力で選択肢を決定する
	if (input.IsTriggered("ok"))
	{
		switch (m_selectIndex)
		{
		case GameoverSelect::Retry:
		{
			//ゲームシーンに遷移する
			m_controller.ChangeScene(
				std::make_shared<GameScene>(
					m_controller), 60.0f);
			break;
		}
		case GameoverSelect::ExitGame:
		{
			//ゲームを終了する
			DxLib_End();
			break;
		}
		}
	}
}

void GameoverScene::Draw()
{
	//ウィンドウサイズ
	Size wsize = Application::GetInstance().GetWindowSize();
	//選択肢の位置
	/*int x = wsize.m_width / 2;
	int y = wsize.m_height / 2;*/

	//とりあえず左上に選択肢を表示する
	//選択中の選択肢に矢印を表示する
	switch (m_selectIndex)
	{
	case GameoverSelect::Retry:
	{
		DrawFormatString(0, 15, 0xffffff, L"→");
		DrawFormatString(15, 15, 0xff0000, L"リトライ");
		DrawFormatString(15, 30, 0xffffff, L"ゲーム終了");
		break;
	}
	case GameoverSelect::ExitGame:
	{
		DrawFormatString(0, 30, 0xffffff, L"→");
		DrawFormatString(15, 15, 0xffffff, L"リトライ");
		DrawFormatString(15, 30, 0xff0000, L"ゲーム終了");
		break;
	}
	}
}