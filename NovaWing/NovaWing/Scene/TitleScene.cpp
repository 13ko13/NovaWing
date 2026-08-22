#include <algorithm>
#include <memory>

#include "TitleScene.h"
#include "Manager/InputManager.h"
#include "SceneController.h"
#include "GameScene.h"
#include "Main/Application.h"
#include "Utility/Size.h"
#include "Utility/SizeF.h"
#include "Utility/GraphShaderDraw.h"
#include "Constants/ShaderRegister.h"
#include "Game/GameObjects/Actors/Charactor/Player/TitlePlayer.h"
#include "Manager/GameObjectManager.h"
#include "Game/GameObjects/Camera/TitleCamera.h"
#include "Game/GameObjects/Camera/CameraBase.h"

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

	//シェーダにフレームを渡すときに値が大きすぎるので小さくするための値
	constexpr float time_speed = 0.1f;

	//ゲームシーンに遷移するときのフェードの長さ
	constexpr float to_gamescene_fade_time = 60.0f;
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
	//グリッチシェーダのロード
	m_glitchPSH = LoadPixelShader(L"GlitchPS.pso");

	//シェーダバッファを作成
	m_cbufferGlitch = CreateShaderConstantBuffer(sizeof(GlitchBuffer));
	m_pCBuffGlitchData = static_cast<GlitchBuffer*>(GetBufferShaderConstantBuffer(m_cbufferGlitch));

	//ゲームオブジェクトマネージャーの初期化
	GameObjectManager::GetInstance().ClearAll();

	//タイトル用のプレイヤーを生成
	m_pPlayer = std::make_shared<TitlePlayer>(
		ResourceLoader::ModelID::Player,
		std::weak_ptr<CameraBase>());//まだカメラは生成されていないので空のカメラを渡す
	//ゲームオブジェクトマネージャーに登録
	m_pPlayer->Init();

	//タイトル用のカメラを生成
	m_pTitleCamera = std::make_shared<TitleCamera>(m_pPlayer);
	//ゲームオブジェクトマネージャーに登録
	m_pTitleCamera->Init();
	//プレイヤーにカメラをセット
	m_pPlayer->SetCamera(m_pTitleCamera);
}

void TitleScene::Update()
{
	//フレーム更新
	m_frame++;
	//シェーダに時間を渡す
	m_pCBuffGlitchData->time = m_frame * time_speed;
	UpdateShaderConstantBuffer(m_cbufferGlitch);

	//InputManagerのインスタンスを取得
	InputManager& input = InputManager::GetInstance();

	//ゲームオブジェクト全ての更新
	GameObjectManager::GetInstance().UpdateAll();

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
					m_controller), to_gamescene_fade_time);
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
	//なによりも背面にゲームオブジェクトを描画させる
	GameObjectManager::GetInstance().DrawAll();

	//ウィンドウサイズ
	Size wsize = Application::GetInstance().GetWindowSize();

	////ロゴを描画
	//DrawRotaGraph(
	//	wsize.m_width * logo_ratio_x,
	//	wsize.m_height * logo_ratio_y,
	//	logo_scale, 0.0, m_titleLogoH, true);

	////シェーダを適用
	//SetUsePixelShader(m_glitchPSH);
	//SetShaderConstantBuffer(m_cbufferGlitch, DX_SHADERTYPE_PIXEL, ShaderRegister::glitch_buffer);

	////選択肢の背景をシェーダを通して描画
	//DrawGraphToShaderByCenter(
	//	wsize.m_width * back_ground_ratio_x,
	//	wsize.m_height * back_ground_ratio_y,
	//	back_ground_graph_scale,
	//	m_selectBackGroundH
	//);

	////選択肢の描画
	//switch (m_selectIndex)
	//{
	//case TitleSelect::StartGame:
	//{
	//	//ゲーム開始選択肢描画
	//	//もしゲームスタートのワイプ進行度が0より大きければ
	//	//カーソルが乗っている画像を左から進行度の範囲だけ切り取って描画
	//	if (m_wipeProgress[static_cast<int>(TitleSelect::StartGame)] > 0.0f)
	//	{
	//		DrawGraphToShaderByCenter(
	//			wsize.m_width * start_ratio_x,
	//			wsize.m_height * start_ratio_y,
	//			start_graph_scale,
	//			m_gameStartOnCursorGraphH,
	//			m_wipeProgress[static_cast<int>(TitleSelect::StartGame)]
	//		);
	//	}

	//	//ゲーム終了選択肢描画
	//	DrawGraphToShaderByCenter(
	//		wsize.m_width * end_ratio_x,
	//		wsize.m_height * end_ratio_y,
	//		end_graph_scale,
	//		m_gameEndGraphH
	//	);
	//	break;
	//}
	//case TitleSelect::ExitGame:
	//{
	//	//ゲーム開始選択肢描画
	//	DrawGraphToShaderByCenter(
	//		wsize.m_width * start_ratio_x,
	//		wsize.m_height * start_ratio_y,
	//		start_graph_scale,
	//		m_gameStartGraphH
	//	);

	//	//ゲーム終了選択肢描画
	//	//もしゲームスタートのワイプ進行度が0より大きければ
	//	//カーソルが乗っている画像を左から進行度の範囲だけ切り取って重ね描きする
	//	if (m_wipeProgress[static_cast<int>(TitleSelect::ExitGame)] > 0.0f)
	//	{
	//		//ゲーム終了選択肢描画
	//		DrawGraphToShaderByCenter(
	//			wsize.m_width * end_ratio_x,
	//			wsize.m_height * end_ratio_y,
	//			end_graph_scale,
	//			m_gameEndOnCursorGraphH,
	//			m_wipeProgress[static_cast<int>(TitleSelect::ExitGame)]
	//		);
	//	}
	//	break;
	//}
	//}

	SetUsePixelShader(-1);
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, ShaderRegister::glitch_buffer);
}
