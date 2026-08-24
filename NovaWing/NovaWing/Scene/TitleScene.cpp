#include <algorithm>
#include <memory>
#include <cmath>

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
#include "Manager/WaterManager.h"
#include "Game/BackGround/SkyBox.h"
#include "Manager/LightingManager.h"

namespace
{
	//ゲームロゴ
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
	//スキャンラインを入れる周期
	constexpr float scanline_frequency = 280.0f;

	//ゲームシーンに遷移するときのフェードの長さ
	constexpr float to_gamescene_fade_time = 60.0f;

	//グリッドのサイズ
	const Vector3 grid_size = { 1400.0f, 0.0f, 1400.0f };

	//プレイヤーを前進させるフレーム
	constexpr int player_forward_max_frame = 120;
	//プレイヤーをブーストさせるフレーム
	constexpr int player_boost_max_frame = 120;

	//タイトルロゴ出現演出にかけるフレーム
	constexpr int logo_max_frame = 30;
	//タイトルロゴのスタンプ演出時の最初の大きさ
	constexpr double logo_max_scale = 3.0;

	//選択肢出現にかけるフレーム
	constexpr int select_max_frame = 50;

	//ライトの方向
	const Vector3 light_direction = Vector3(1.0f, -1.0f, 0.6f);
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
	//スキャンラインを入れる周期をシェーダに渡す
	m_pCBuffGlitchData->scanlineFrequency = scanline_frequency;
	UpdateShaderConstantBuffer(m_cbufferGlitch);

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

	//水マネージャーの初期化
	m_pWaterManager = std::make_shared<WaterManager>(m_pTitleCamera);
	m_pWaterManager->Init();

	//スカイボックスの初期化
	m_pSkyBox = std::make_shared<SkyBox>(
		loader.GetGraphic(ResourceLoader::GraphicID::SkyBoxFront),
		loader.GetGraphic(ResourceLoader::GraphicID::SkyBoxBack),
		loader.GetGraphic(ResourceLoader::GraphicID::SkyBoxLeft),
		loader.GetGraphic(ResourceLoader::GraphicID::SkyBoxRight),
		loader.GetGraphic(ResourceLoader::GraphicID::SkyBoxUp),
		loader.GetGraphic(ResourceLoader::GraphicID::SkyBoxBottom)
	);

	// ライトの方向ベクトルをセットする
	LightingManager::GetInstance().SetLightDirection(light_direction);
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

	//水マネージャーの更新
	m_pWaterManager->Update();

	
	
	switch (m_phase)
	{
	case Phase::Forward:
		//一定フレーム数プレイヤーに前進させる
		if (m_frame == player_forward_max_frame)
		{
			//フレームが達したら宙返りステートに遷移
			m_pPlayer->StartSomersault();
			m_phase = Phase::Somersault;
		}

		break;

	case Phase::Somersault:
		if (m_pPlayer->IsSomersaultEnd())
		{
			//宙返りが終っていたらブーストステートに遷移
			m_pPlayer->StartBoost();
			m_phase = Phase::Boost;
		}
		break;
	case Phase::Boost:
		//一定フレームブーストしたらタイトルロゴ出現させる
		m_playerBoostFrame++;
		if (m_playerBoostFrame == player_boost_max_frame)
		{
			//ロゴステートに遷移
			m_phase = Phase::LogoAndSelect;
			//カメラも追従をやめる
			m_pTitleCamera->StopFollowing();
		}
		break;

	case Phase::LogoAndSelect:
		//ロゴ演出用のフレーム更新
		if (logo_max_frame > m_titleLogoFrame)
		{
			m_titleLogoFrame++;
			
		}
		if (select_max_frame > m_selectFadeFrame)
		{
			//選択肢も同様にフレーム更新
			m_selectFadeFrame++;
		}

		//選択肢のカーソルもこのフェーズ以外では触れないようにする
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
		break;
	}

	//演出スキップ
	if (m_phase != Phase::LogoAndSelect)
	{
		if (input.IsTriggered(InputEvent::ok))
		{
			m_phase = Phase::LogoAndSelect;
		}
	}

	//ゲームオブジェクト全ての更新
	GameObjectManager::GetInstance().UpdateAll();
}

void TitleScene::Draw()
{
#ifdef _DEBUG
	//直線の始点と終点
	VECTOR startPos;
	VECTOR endPos;

	//ステージのサイズに合わせてグリッドを描画する
	for (int z = static_cast<int>(-grid_size.m_z);
		z <= static_cast<int>(grid_size.m_z); z += 100)
	{
		startPos = VGet(-grid_size.m_x, 0.0f, static_cast<float>(z));
		endPos = VGet(grid_size.m_x, 0.0f, static_cast<float>(z));
		DrawLine3D(startPos, endPos, 0xff0000);
	}
	for (int x = static_cast<int>(-grid_size.m_x); x <= static_cast<int>(grid_size.m_x); x += 100)
	{
		startPos = VGet(static_cast<float>(x), 0.0f, -grid_size.m_z);
		endPos = VGet(static_cast<float>(x), 0.0f, grid_size.m_z);
		DrawLine3D(startPos, endPos, 0x0000ff);
	}
#endif
	//スカイボックスの描画
	m_pSkyBox->Draw(m_pTitleCamera->GetPos());

	//水マネージャーの描画
	m_pWaterManager->Draw();

	//ゲームオブジェクトを描画させる
	GameObjectManager::GetInstance().DrawAll();

	//ウィンドウサイズ
	Size wsize = Application::GetInstance().GetWindowSize();


	//ロゴ演出フェーズになってからロゴを描画
	if (m_phase == Phase::LogoAndSelect)
	{
		//タイトルロゴの演出用進行度計算
		double progress = static_cast<double>(m_titleLogoFrame) / logo_max_frame;
		//最初の大きさから通常の大きさに補完する
		m_titleLogoScale = std::lerp(logo_max_scale, 1.0, progress);

		//タイトルロゴ描画
		DrawRotaGraph(
			wsize.m_width * logo_ratio_x,
			wsize.m_height * logo_ratio_y,
			m_titleLogoScale, 0.0, m_titleLogoH, true);

		//ちょっと遅めに選択肢も出現させる
		float selectProgress = static_cast<float>(m_selectFadeFrame) /
			static_cast<float>(select_max_frame);

		//シェーダを適用
		SetUsePixelShader(m_glitchPSH);
		SetShaderConstantBuffer(m_cbufferGlitch, DX_SHADERTYPE_PIXEL, ShaderRegister::glitch_buffer);

		//選択肢の背景をシェーダを通して描画
		DrawGraphToShaderByCenter(
			wsize.m_width * back_ground_ratio_x,
			wsize.m_height * back_ground_ratio_y,
			back_ground_graph_scale,
			m_selectBackGroundH,
			selectProgress
		);

		//選択肢の描画
		switch (m_selectIndex)
		{
		case TitleSelect::StartGame:
		{
			//ゲーム開始選択肢描画
			//もしゲームスタートのワイプ進行度が0より大きければ
			//カーソルが乗っている画像を左から進行度の範囲だけ切り取って描画
			if (m_wipeProgress[static_cast<int>(TitleSelect::StartGame)] > 0.0f)
			{
				DrawGraphToShaderByCenter(
					wsize.m_width * start_ratio_x,
					wsize.m_height * start_ratio_y,
					start_graph_scale,
					m_gameStartOnCursorGraphH,
					selectProgress,
					m_wipeProgress[static_cast<int>(TitleSelect::StartGame)]
				);
			}

			//ゲーム終了選択肢描画
			DrawGraphToShaderByCenter(
				wsize.m_width * end_ratio_x,
				wsize.m_height * end_ratio_y,
				end_graph_scale,
				m_gameEndGraphH,
				selectProgress
			);
			break;
		}
		case TitleSelect::ExitGame:
		{
			//ゲーム開始選択肢描画
			DrawGraphToShaderByCenter(
				wsize.m_width * start_ratio_x,
				wsize.m_height * start_ratio_y,
				start_graph_scale,
				m_gameStartGraphH,
				selectProgress
			);

			//ゲーム終了選択肢描画
			//もしゲームスタートのワイプ進行度が0より大きければ
			//カーソルが乗っている画像を左から進行度の範囲だけ切り取って重ね描きする
			if (m_wipeProgress[static_cast<int>(TitleSelect::ExitGame)] > 0.0f)
			{
				//ゲーム終了選択肢描画
				DrawGraphToShaderByCenter(
					wsize.m_width * end_ratio_x,
					wsize.m_height * end_ratio_y,
					end_graph_scale,
					m_gameEndOnCursorGraphH,
					selectProgress,
					m_wipeProgress[static_cast<int>(TitleSelect::ExitGame)]
				);
			}
			break;
		}
		}
	}

	SetUsePixelShader(-1);
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, ShaderRegister::glitch_buffer);
}
