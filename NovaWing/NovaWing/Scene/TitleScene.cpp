#include <algorithm>

#include "TitleScene.h"
#include "Manager/InputManager.h"
#include "SceneController.h"
#include "GameScene.h"
#include "Main/Application.h"

namespace
{
	//ゲームロゴ
	constexpr double logo_scale = 1.0;//ロゴのサイズ
	constexpr float logo_ratio_x = 0.5f;//画面に対して横位置をどのあたりにしたいか
	constexpr float logo_ratio_y = 0.3f;//画面に対して縦位置をどのあたりにしたいか

	//ゲーム開始選択肢
	constexpr float start_ratio_x = 0.5f;//画面に対して横位置をどのあたりにしたいか
	constexpr float start_ratio_y = 0.74f;//画面に対して縦位置をどのあたりにしたいか
	constexpr double start_graph_scale = 0.55;//選択肢画像のサイズ

	//ゲーム終了選択肢
	constexpr float end_ratio_x = 0.5f;//画面に対して横位置をどのあたりにしたいか
	constexpr float end_ratio_y = 0.85;//画面に対して縦位置をどのあたりにしたいか
	constexpr double end_graph_scale = 0.55;//選択肢画像のサイズ

	//選択肢の背景画像
	constexpr float back_ground_ratio_x = 0.5f;//画面に対して横位置をどのあたりにしたいか
	constexpr float back_ground_ratio_y = 0.77f;//画面に対して縦位置をどのあたりにしたいか
	constexpr double back_ground_graph_scale = 1.0;//背景画像のサイズ

	constexpr int wipe_max = 10;//カーソルが乗った時に何秒で画像を切り替えるか
}

TitleScene::TitleScene(SceneController& controller):
	Scene(controller)
{
	
}

TitleScene::~TitleScene()
{
}

void TitleScene::Init()
{
	//ローダー
	ResourceLoader& loader = ResourceLoader::GetInstance();

	//タイトルロゴをロード
	m_titleLogoH = loader.GetGraphic(
		ResourceLoader::GraphicID::TitleLogo);
	//ゲーム開始画像をロード
	m_gameStartGraphH = loader.GetGraphic(
		ResourceLoader::GraphicID::GameStart);
	//ゲーム終了画像をロード
	m_gameEndGraphH = loader.GetGraphic(
		ResourceLoader::GraphicID::GameEnd);
	//ゲーム開始画像をロード
	m_gameStartOnCursorGraphH = loader.GetGraphic(
		ResourceLoader::GraphicID::GameStartOnCursor);
	//ゲーム終了画像をロード
	m_gameEndOnCursorGraphH = loader.GetGraphic(
		ResourceLoader::GraphicID::GameEndOnCursor);
	//選択肢の背景画像をロード
	m_selectBackGroundH = loader.GetGraphic(
		ResourceLoader::GraphicID::SelectBackGround);
}

void TitleScene::Update()
{
	//フレーム更新
	m_frame++;

	//InputManagerのインスタンスを取得
	InputManager& input = InputManager::GetInstance();

	//下入力で選択肢を下に移動(indexを増やす) 
	if (input.IsTriggered(InputEvent::down))
	{
		//選択肢の最大数で割った余りを取ることで、
		//選択肢の範囲内に収める
		m_selectIndex =
			static_cast<TitleSelect>(
				(static_cast<int>(m_selectIndex) + 1) %
				static_cast<int>(TitleSelect::SelectMax
					));
	}
	//上入力で選択肢を上に移動(indexを減らす)
	if (input.IsTriggered(InputEvent::up))
	{
		//選択肢の最大数で割った余りを取ることで、
		//選択肢の範囲内に収める
		m_selectIndex =
			static_cast<TitleSelect>(
				(static_cast<int>(m_selectIndex) - 1 + static_cast<int>(TitleSelect::SelectMax)) %
				static_cast<int>(TitleSelect::SelectMax)
				);
	}
	//決定入力で選択肢を決定する
	if (input.IsTriggered(InputEvent::ok))
	{
		switch (m_selectIndex)
		{
		case TitleSelect::StartGame:
		{
			//ゲームシーンに遷移する
			m_controller.ChangeScene(
				std::make_shared<GameScene>(
					m_controller), 60.0f);
			break;
		}
		case TitleSelect::ExitGame:
		{
			//アプリケーションに終了を要求する
			Application::GetInstance().RequestExit();
			break;
		}
		}
	}

	//現在の選択肢と前のフレームの選択肢を比較して
	//変わっていたらワイプの進行度をリセット
	if (m_selectIndex != m_prevSelectIdx)
	{
		m_wipeProgress[static_cast<int>(m_selectIndex)] = 0.0f;
	}
	//選択肢が一致しているワイプ進行度を増やす
	//一致しないもののワイプ進行度を減らす
	for (int i = 0; i < static_cast<int>(TitleSelect::SelectMax); i++)
	{
		//今回見たい選択肢
		TitleSelect current = static_cast<TitleSelect>(i);//iをTitleSelectの番号としてみる
		//選択肢が一致している場合
		if (current == m_selectIndex)
		{
			m_wipeProgress[i] += 1.0f / wipe_max;
			
		}
		else//一致しない場合
		{
			m_wipeProgress[i] -= 1.0f / wipe_max;
		}
		m_wipeProgress[i] = std::clamp(m_wipeProgress[i], 0.0f, 1.0f);//0~1にクランプ
	}

	//前フレームの選択肢を保存
	m_prevSelectIdx = m_selectIndex;
}

void TitleScene::Draw()
{
	//ウィンドウサイズ
	Size wsize = Application::GetInstance().GetWindowSize();

	//ロゴを描画
	DrawRotaGraph(
		wsize.m_width * logo_ratio_x,
		wsize.m_height * logo_ratio_y,
		logo_scale, 0.0, m_titleLogoH, true);

	//選択肢の背景
	DrawRotaGraph(
		wsize.m_width * back_ground_ratio_x,
		wsize.m_height * back_ground_ratio_y,
		back_ground_graph_scale, 0.0, m_selectBackGroundH, true);

	//画像の幅と高さ
	int graphWidth, graphHeight;
	GetGraphSize(m_gameStartGraphH, &graphWidth, &graphHeight);
	//画像の中心
	int graphCenter = wsize.m_width * start_ratio_x;
	//画像の左端のX座標
	int leftX = graphCenter - (graphWidth * start_graph_scale) / 2;

	//とりあえず左上に選択肢を表示する
	//選択中の選択肢に矢印を表示する
	switch (m_selectIndex)
	{
	case TitleSelect::StartGame:
	{
		//ゲーム開始選択肢描画
		//もしゲームスタートのワイプ進行度が0より大きければ
		//カーソルが乗っている画像を左から進行度の範囲だけ切り取って描画
		if (m_wipeProgress[static_cast<int>(TitleSelect::StartGame)] > 0.0f)
		{
			DrawRectRotaGraph(
			leftX + (graphWidth * m_wipeProgress[static_cast<int>(TitleSelect::StartGame)] *
			start_graph_scale / 2),
			wsize.m_height * start_ratio_y,
			0,0,
			graphWidth * m_wipeProgress[static_cast<int>(m_selectIndex)],
			graphHeight,
			start_graph_scale, 0.0, m_gameStartOnCursorGraphH, true);
		}

		//ゲーム終了選択肢描画
		DrawRotaGraph(
		wsize.m_width * end_ratio_x,
		wsize.m_height * end_ratio_y,
		end_graph_scale, 0.0, m_gameEndGraphH, true);
		break;
	}
	case TitleSelect::ExitGame:
	{
		//ゲーム開始選択肢描画
		DrawRotaGraph(
		wsize.m_width * start_ratio_x,
		wsize.m_height * start_ratio_y,
		start_graph_scale, 0.0, m_gameStartGraphH, true);

		//ゲーム終了選択肢描画
		//もしゲームスタートのワイプ進行度が0より大きければ
		//カーソルが乗っている画像を左から進行度の範囲だけ切り取って重ね描きする
		if (m_wipeProgress[static_cast<int>(TitleSelect::ExitGame)] > 0.0f)
		{
			DrawRectRotaGraph(
			leftX + (graphWidth * m_wipeProgress[static_cast<int>(TitleSelect::ExitGame)] * end_graph_scale) / 2,
			wsize.m_height * end_ratio_y,
			0,0,
			graphWidth * m_wipeProgress[static_cast<int>(m_selectIndex)],
			graphHeight,
			end_graph_scale, 0.0, m_gameEndOnCursorGraphH, true);
		}
		break;
	}
	}
}
