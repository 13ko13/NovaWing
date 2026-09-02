#include <DxLib.h>
#include <algorithm>

#include "PauseScene.h"
#include "SceneController.h"
#include "Manager/InputManager.h"
#include "Main/Application.h"
#include "Constants/ShaderRegister.h"
#include "Manager/ResourceLoader.h"
#include "Scene/TitleScene.h"
#include "Utility/GraphShaderDraw.h"
#include "Manager/SoundManager.h"

namespace
{
	//スキャンラインを入れる周期
	constexpr float scanline_frequency = 280.0f;
	//選択肢の背景画像
	constexpr float back_ground_ratio_x = 0.5f;//画面に対して横位置をどのあたりにしたいか
	constexpr float back_ground_ratio_y = 0.5f;//画面に対して縦位置をどのあたりにしたいか
	constexpr double back_ground_graph_scale = 1.5;//背景画像のサイズ
	//背景の開く時間
	constexpr int background_opne_max_frame = 15;
	//サイズ
	constexpr double select_graph_scale = 0.8;
	//ゲームに戻るの選択肢の場所
	const Vector2 back_game_ratio = Vector2(0.5f, 0.45f);
	//タイトルに戻るの選択肢の場所
	const Vector2 back_title_ratio = Vector2(0.5f, 0.6f);
	//シェーダに渡すときに、早すぎるため倍率を低くする
	constexpr float time_speed = 0.1f;
	//ポーズ中の黒画像のアルファ
	constexpr float black_graph_alpha = 126;
}

PauseScene::PauseScene(SceneController& controller, std::weak_ptr<SoundManager> pSoundManager) :
	Scene(controller),
	m_pSoundManager(pSoundManager)
{
}

PauseScene::~PauseScene()
{
}

void PauseScene::Init()
{
	//グリッチシェーダのロード
	m_glitchPSH = LoadPixelShader(L"GlitchPS.pso");

	//シェーダバッファを作成
	m_cbufferGlitch = CreateShaderConstantBuffer(sizeof(GlitchBuffer));
	m_pCBuffGlitchData = static_cast<GlitchBuffer*>(GetBufferShaderConstantBuffer(m_cbufferGlitch));
	//スキャンラインを入れる周期をシェーダに渡す
	m_pCBuffGlitchData->scanlineFrequency = scanline_frequency;
	UpdateShaderConstantBuffer(m_cbufferGlitch);
}

void PauseScene::Update()
{
	//フレーム更新
	m_frame++;
	m_pCBuffGlitchData->time = m_frame * time_speed;
	UpdateShaderConstantBuffer(m_cbufferGlitch);

	InputManager& input = InputManager::GetInstance();

	//借りているサウンドマネージャー(GameScene所有)。ポーズ中はGameScene::Updateが止まるので
	//フェード等は進まないが、単発SEの再生には問題ない
	auto pSound = m_pSoundManager.lock();

	//選択肢のカーソルもこのフェーズ以外では触れないようにする
	//下入力で選択肢を下に移動(indexを増やす) 
	if (input.IsTriggered(InputEvent::down))
	{
		//カーソルが乗った時の音を鳴らす
		if (pSound) pSound->Play(SoundManager::SoundType::OnCursor);

		//選択肢の最大数で割った余りを取ることで、
		//選択肢の範囲内に収める
		m_select =
			static_cast<Select>(
				(static_cast<int>(m_select) + 1) %
				static_cast<int>(Select::Max
					));
	}
	//上入力で選択肢を上に移動(indexを減らす)
	if (input.IsTriggered(InputEvent::up))
	{
		//カーソルが乗った時の音を鳴らす
		if (pSound) pSound->Play(SoundManager::SoundType::OnCursor);

		//選択肢の最大数で割った余りを取ることで、
		//選択肢の範囲内に収める
		m_select =
			static_cast<Select>(
				(static_cast<int>(m_select) - 1 + static_cast<int>(Select::Max)) %
				static_cast<int>(Select::Max)
				);
	}

	//選択肢背景の開く演出用のフレーム更新
	m_backGroundOpenFrame++;
	if (m_backGroundOpenFrame > background_opne_max_frame)
	{
		m_backGroundOpenFrame = background_opne_max_frame;
	}

	//背景が開ききっているときだけ決定入力を受け付ける
	if (m_backGroundOpenFrame >= background_opne_max_frame &&
		input.IsTriggered(InputEvent::ok))
	{
		//決定音を鳴らす
		if (pSound) pSound->Play(SoundManager::SoundType::Decision);

		switch (m_select)
		{
		case Select::BackGame:
		{
			//ゲームに戻る(ポーズシーンを閉じるだけ)
			m_controller.PopScene();
			break;
		}
		case Select::BackTitle:
		{
			//タイトルシーンに遷移する
			m_controller.ChangeScene(
				std::make_shared<TitleScene>(
					m_controller), 60.0f);
			break;
		}
		}

	}
	if (m_backGroundOpenFrame >= background_opne_max_frame)
	{
		//Bボタンでも閉じるようにする
		if (input.IsTriggered(InputEvent::close))
		{
			//決定音を鳴らす
			if (pSound) pSound->Play(SoundManager::SoundType::Decision);
			//ゲームに戻る(ポーズシーンを閉じるだけ)
			m_controller.PopScene();
		}
	}
	//現在の選択肢と前のフレームの選択肢を比較して
	//変わっていたらワイプの進行度をリセット
	if (m_select != m_prevSelectIdx)
	{
		m_wipeProgress[static_cast<int>(m_select)] = 0.0f;
	}
	//選択肢が一致しているワイプ進行度を増やす
	//一致しないもののワイプ進行度を減らす
	for (int i = 0; i < static_cast<int>(Select::Max); i++)
	{
		Select current = static_cast<Select>(i);
		if (current == m_select)
		{
			m_wipeProgress[i] += 1.0f / 15.0f;
		}
		else
		{
			m_wipeProgress[i] -= 1.0f / 15.0f;
		}
		m_wipeProgress[i] = std::clamp(m_wipeProgress[i], 0.0f, 1.0f);
	}

	//前フレームの選択肢を保存
	m_prevSelectIdx = m_select;
}

void PauseScene::Draw()
{
	const auto& wsize = Application::GetInstance().GetWindowSize();

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, black_graph_alpha);
	DrawBox(0, 0, wsize.m_width, wsize.m_height, 0x000000, true);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

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
		wsize.m_width * back_ground_ratio_x, wsize.m_height * back_ground_ratio_y,
		back_ground_graph_scale, backgroundH,
		1.0f,
		uvMaxU,
		uvMinU
	);

	//ゲームに戻る選択肢画像
	int backGameHandle = loader.GetGraphic(ResourceLoader::GraphicID::BackGame);
	//タイトルに戻る選択肢画像
	int backTitleHandle = loader.GetGraphic(ResourceLoader::GraphicID::BackTitle);
	//カーソルが乗っているときのゲームに戻る選択肢画像
	int backGameOnCursorHandle = loader.GetGraphic(ResourceLoader::GraphicID::BackGameOnCursor);
	//カーソルが乗っているときのタイトルに戻る選択肢画像
	int backTitleOnCursorHandle = loader.GetGraphic(ResourceLoader::GraphicID::BackTitleOnCursor);

	//選択肢の描画
	if (openProgress != 1.0f)
	{
		//ゲームに戻る選択肢を描画
		DrawGraphToShaderByCenter(
			wsize.m_width * back_game_ratio.m_x,
			wsize.m_height * back_game_ratio.m_y,
			select_graph_scale, backGameHandle,
			1.0f,
			uvMaxU,
			uvMinU
		);
		//タイトルに戻る選択肢を描画
		DrawGraphToShaderByCenter(
			wsize.m_width * back_title_ratio.m_x,
			wsize.m_height * back_title_ratio.m_y,
			select_graph_scale, backTitleHandle,
			1.0f,
			uvMaxU,
			uvMinU
		);
	}
	else
	{
		switch (m_select)
		{
		case Select::BackGame:
		{
			if (m_wipeProgress[static_cast<int>(Select::BackGame)] > 0.0f)
			{
				DrawGraphToShaderByCenter(
					wsize.m_width * back_game_ratio.m_x,
					wsize.m_height * back_game_ratio.m_y,
					select_graph_scale, backGameOnCursorHandle,
					1.0f,
					m_wipeProgress[static_cast<int>(Select::BackGame)]
				);
			}
			DrawGraphToShaderByCenter(
				wsize.m_width * back_title_ratio.m_x,
				wsize.m_height * back_title_ratio.m_y,
				select_graph_scale, backTitleHandle,
				1.0f
			);
			break;
		}
		case Select::BackTitle:
		{
			if (m_wipeProgress[static_cast<int>(Select::BackTitle)] > 0.0f)
			{
				DrawGraphToShaderByCenter(
					wsize.m_width * back_title_ratio.m_x,
					wsize.m_height * back_title_ratio.m_y,
					select_graph_scale, backTitleOnCursorHandle,
					1.0f, m_wipeProgress[static_cast<int>(Select::BackTitle)]
				);
			}
			DrawGraphToShaderByCenter(
				wsize.m_width * back_game_ratio.m_x,
				wsize.m_height * back_game_ratio.m_y,
				select_graph_scale, backGameHandle,
				1.0f
			);
			break;
		}
		}
	}

	SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, ShaderRegister::glitch_buffer);
	SetUsePixelShader(-1);
}
