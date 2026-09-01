#include <algorithm>

#include "GameoverScene.h"
#include "Manager/InputManager.h"
#include "SceneController.h"
#include "GameScene.h"
#include "Main/Application.h"
#include "Manager/ResourceLoader.h"
#include "Utility/GraphShaderDraw.h"
#include "Constants/ShaderRegister.h"
#include "Manager/SoundManager.h"

namespace
{
	//決定キーの遷移先に切り替わるまでのフレーム数
	constexpr float scene_change_frame = 60.0f;

	//シェーダにフレームを渡すときに値が大きすぎるので小さくするための値
	constexpr float time_speed = 0.1f;
	//スキャンラインを入れる周期
	constexpr float scanline_frequency = 280.0f;

	//選択肢の背景画像の開く演出を何フレームかけるか
	constexpr int background_opne_max_frame = 30;

	//選択肢画像のサイズ
	constexpr double select_graph_scale = 1.2;
	//選択肢の背景画像のサイズ
	constexpr double background_graph_scale = 2.25;

	//リトライ選択肢の位置
	const Vector2 retry_ratio = Vector2(0.5f, 0.43f);//画面に対してどのあたりにしたいか
	//ゲーム終了選択肢の位置
	const Vector2 exit_game_ratio = Vector2(0.5f, 0.63f);//画面に対してどのあたりにしたいか

	//カーソルが乗った時に何フレームで画像を切り替えるか
	constexpr int wipe_max = 10;

	//決定画像のウィンドウに対する位置の割合
	const Vector2 decide_graph_pos_ratio = Vector2(0.9297f, 0.9583f);
	//Aボタン画像のサイズ
	constexpr double a_button_scale = 0.45;
	//決定のテキスト画像のサイズ
	constexpr double decide_graph_scale = 0.45;

	//aボタン画像のウィンドウに対する位置の割合
	const Vector2 a_button_pos_ratio = { 0.8984f,0.9583f };
}

GameoverScene::GameoverScene(SceneController& controller):
	Scene(controller)
{
}

GameoverScene::~GameoverScene()
{
}

void GameoverScene::Init()
{
	//サウンドマネージャーの初期化
	m_pSoundManager = std::make_shared<SoundManager>();
	m_pSoundManager->Init();

	//ゲームオーバーBGMを鳴らす
	m_pSoundManager->Play(SoundManager::SoundType::GameoverBGM, true);

	//グリッチシェーダのロード
	m_glitchPSH = LoadPixelShader(L"GlitchPS.pso");

	//シェーダバッファを作成
	m_cbufferGlitch = CreateShaderConstantBuffer(sizeof(GlitchBuffer));
	m_pCBuffGlitchData = static_cast<GlitchBuffer*>(GetBufferShaderConstantBuffer(m_cbufferGlitch));
	//スキャンラインを入れる周期をシェーダに渡す
	m_pCBuffGlitchData->scanlineFrequency = scanline_frequency;
	UpdateShaderConstantBuffer(m_cbufferGlitch);
}

void GameoverScene::Update()
{
	//InputManagerのインスタンスを取得
	InputManager& input = InputManager::GetInstance();
	//フレーム更新
	m_frame++;
	m_pCBuffGlitchData->time = m_frame * time_speed;//シェーダ用のフレームに変換して渡す
	UpdateShaderConstantBuffer(m_cbufferGlitch);

	//サウンドマネージャーの更新
	m_pSoundManager->Update();

	//選択肢背景の開く演出用のフレーム更新
	if (!m_controller.GetFade().IsFading())
	{
		m_backGroundOpenFrame++;
		if (m_backGroundOpenFrame > background_opne_max_frame)
		{
			m_backGroundOpenFrame = background_opne_max_frame;
		}
	}

	//下入力で選択肢を下に移動(indexを増やす)
	if (input.IsTriggered(InputEvent::down))
	{
		//選択肢の最大数で割った余りを取ることで、
		//選択肢の範囲内に収める
		m_selectIndex =
			static_cast<GameoverSelect>(
				(static_cast<int>(m_selectIndex) + 1) %
				static_cast<int>(GameoverSelect::SelectMax));
		//選択音を鳴らす
		m_pSoundManager->Play(SoundManager::SoundType::OnCursor);
	}
	//上入力で選択肢を上に移動(indexを減らす)
	if (input.IsTriggered(InputEvent::up))
	{
		//選択肢の最大数で割った余りを取ることで、
		//選択肢の範囲内に収める
		m_selectIndex =
			static_cast<GameoverSelect>(
				(static_cast<int>(m_selectIndex) - 1 + static_cast<int>(GameoverSelect::SelectMax)) %
				static_cast<int>(GameoverSelect::SelectMax));
		//選択音を鳴らす
		m_pSoundManager->Play(SoundManager::SoundType::OnCursor);
	}
	//背景が開ききっているときだけ決定入力を受け付ける
	if (m_backGroundOpenFrame >= background_opne_max_frame &&
		input.IsTriggered(InputEvent::ok))
	{
		//決定音を鳴らす
		m_pSoundManager->Play(SoundManager::SoundType::Decision);

		switch (m_selectIndex)
		{
		case GameoverSelect::Retry:
		{
			//ゲームシーンに遷移する
			m_controller.ChangeScene(
				std::make_shared<GameScene>(
					m_controller), scene_change_frame);
			break;
		}
		case GameoverSelect::ExitGame:
		{
			//ゲームを終了する
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
	for (int i = 0; i < static_cast<int>(GameoverSelect::SelectMax); i++)
	{
		//今回見たい選択肢
		GameoverSelect current = static_cast<GameoverSelect>(i);//iをGameoverSelectの番号としてみる
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

void GameoverScene::Draw()
{
	//ウィンドウサイズ
	Size wsize = Application::GetInstance().GetWindowSize();
	ResourceLoader& loader = ResourceLoader::GetInstance();

	SetUsePixelShader(m_glitchPSH);
	SetShaderConstantBuffer(m_cbufferGlitch, DX_SHADERTYPE_PIXEL, ShaderRegister::glitch_buffer);

	//画像をカーテンのように開く感じで表示するために
	//進行度計算
	float openProgress = static_cast<float>(m_backGroundOpenFrame) /
		static_cast<float>(background_opne_max_frame);

	//中心を基準に左右対称の範囲を計算
	float uvMinU = 0.5f - openProgress * 0.5f;
	float uvMaxU = 0.5f + openProgress * 0.5f;

	//選択肢背景画像
	int backgroundH = loader.GetGraphic(ResourceLoader::GraphicID::SelectBackGround);
	DrawGraphToShaderByCenter(
		wsize.m_width * 0.5f, wsize.m_height * 0.5f,
		background_graph_scale, backgroundH,
		1.0f,
		uvMaxU,
		uvMinU
	);

	//リトライ選択肢画像
	int retryHandle = loader.GetGraphic(ResourceLoader::GraphicID::ReTry);
	//ゲーム終了選択肢画像
	int exitGameHandle = loader.GetGraphic(ResourceLoader::GraphicID::GameEnd);
	//カーソルが乗っているときのリトライ選択肢画像
	int retryOnCursorHandle = loader.GetGraphic(ResourceLoader::GraphicID::ReTryOnCursor);
	//カーソルが乗っているときのゲーム終了選択肢画像
	int exitGameOnCursorHandle = loader.GetGraphic(ResourceLoader::GraphicID::GameEndOnCursor);

	//選択肢の描画
	//開く演出中(openProgressが1未満)は、まだワイプを考慮せず
	//通常画像2つを開く演出の範囲(uvMaxU/uvMinU)で描画する
	if (openProgress != 1.0f)
	{
		//リトライ選択肢を描画
		DrawGraphToShaderByCenter(
			wsize.m_width * retry_ratio.m_x,
			wsize.m_height * retry_ratio.m_y,
			select_graph_scale, retryHandle,
			1.0f,
			uvMaxU,
			uvMinU
		);
		//ゲーム終了選択肢を描画
		DrawGraphToShaderByCenter(
			wsize.m_width * exit_game_ratio.m_x,
			wsize.m_height * exit_game_ratio.m_y,
			select_graph_scale, exitGameHandle,
			1.0f,
			uvMaxU,
			uvMinU
		);
	}
	//開く演出が終わっていたら、選ばれている方だけカーソルオン画像を
	//ワイプ進行度で重ね描きし、選ばれていない方は常にフル表示する
	else
	{
		switch (m_selectIndex)
		{
		case GameoverSelect::Retry:
		{
			//リトライ選択肢描画
			//もしリトライのワイプ進行度が0より大きければ
			//カーソルが乗っている画像を左から進行度の範囲だけ切り取って描画
			if (m_wipeProgress[static_cast<int>(GameoverSelect::Retry)] > 0.0f)
			{
				//リトライ選択肢を描画
				DrawGraphToShaderByCenter(
					wsize.m_width * retry_ratio.m_x,
					wsize.m_height * retry_ratio.m_y,
					select_graph_scale, retryOnCursorHandle,
					1.0f,
					m_wipeProgress[static_cast<int>(GameoverSelect::Retry)]
				);
			}
			//ゲーム終了選択肢を描画
			DrawGraphToShaderByCenter(
				wsize.m_width * exit_game_ratio.m_x,
				wsize.m_height * exit_game_ratio.m_y,
				select_graph_scale, exitGameHandle,
				1.0f
			);
			break;
		}
		case GameoverSelect::ExitGame:
		{
			//ゲーム終了のほうのワイプ進行度が0より大きければ
			//ゲーム終了の方に進行度を適用する
			if (m_wipeProgress[static_cast<int>(GameoverSelect::ExitGame)] > 0.0f)
			{
				//ゲーム終了選択肢を描画
				DrawGraphToShaderByCenter(
					wsize.m_width * exit_game_ratio.m_x,
					wsize.m_height * exit_game_ratio.m_y,
					select_graph_scale, exitGameOnCursorHandle,
					1.0f, m_wipeProgress[static_cast<int>(GameoverSelect::ExitGame)]
				);
			}
			//リトライ選択肢を描画
			DrawGraphToShaderByCenter(
				wsize.m_width * retry_ratio.m_x,
				wsize.m_height * retry_ratio.m_y,
				select_graph_scale, retryHandle,
				1.0f
			);
			break;
		}
		}
	}

	SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, ShaderRegister::glitch_buffer);
	SetUsePixelShader(-1);

	//Aボタン画像
	int aButtonHandle = loader.GetGraphic(ResourceLoader::GraphicID::ButtonA);
	//決定のテキスト画像
	int decideHandle = loader.GetGraphic(ResourceLoader::GraphicID::DecideText);

	//aボタン画像の描画位置
	Vector2 aButtonDrawPos = Vector2(
		wsize.m_width * a_button_pos_ratio.m_x,
		wsize.m_height * a_button_pos_ratio.m_y
	);
	//決定画像描画位置
	Vector2 decideDrawPos = Vector2(
		wsize.m_width * decide_graph_pos_ratio.m_x,
		wsize.m_height * decide_graph_pos_ratio.m_y
	);

	//二つの画像を描画
	//Aボタン画像
	DrawRotaGraph(
		aButtonDrawPos.m_x,
		aButtonDrawPos.m_y,
		a_button_scale, 0.0, aButtonHandle, true
	);
	//決定のテキスト画像
	DrawRotaGraph(
		decideDrawPos.m_x,
		decideDrawPos.m_y,
		decide_graph_scale, 0.0, decideHandle, true
	);
}
