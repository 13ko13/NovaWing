#include <DxLib.h>
#include <algorithm>

#include "PauseScene.h"
#include "SceneID.h"
#include "SceneController.h"
#include "Manager/InputManager.h"
#include "Main/Application.h"
#include "Constants/ShaderRegister.h"
#include "Manager/ResourceLoader.h"
#include "Scene/TitleScene.h"
#include "Scene/GameScene.h"
#include "Utility/GraphShaderDraw.h"
#include "Manager/SoundManager.h"
#include "Manager/DebugManager.h"
#include "Game/GameObjects/Actors/Character/Player/Player.h"

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
#ifdef _DEBUG
	//デバッグ表示ON/OFF選択肢の場所
	const Vector2 debug_toggle_ratio = Vector2(0.5f, 0.75f);
	//最初からやり直す選択肢の場所
	const Vector2 restart_ratio = Vector2(0.5f, 0.85f);
	//プレイヤーを移動選択肢の場所
	const Vector2 warp_player_ratio = Vector2(0.5f, 0.9f);

	//物差しのワープ範囲(プレイヤーの初期位置Z 〜 ボス出現位置Z)
	constexpr float warp_range_min_z = 1200.0f;
	constexpr float warp_range_max_z = 27000.0f;
	//物差しの見た目の左右端の画面比率
	constexpr float warp_ruler_ratio_left = 0.15f;
	constexpr float warp_ruler_ratio_right = 0.85f;
	//物差しを表示する高さの画面比率
	constexpr float warp_ruler_ratio_y = 0.5f;
	//左スティックでカーソルを動かす速さ(1フレームあたりの割合)
	constexpr float warp_cursor_move_speed = 0.01f;
	//左スティックの入力とみなすしきい値
	constexpr int warp_stick_threshold = 100;
#endif
	//シェーダに渡すときに、早すぎるため倍率を低くする
	constexpr float time_speed = 0.1f;
	//ポーズ中の黒画像のアルファ
	constexpr int black_graph_alpha = 126;
}

PauseScene::PauseScene(
	SceneController& controller,
	std::weak_ptr<SoundManager> pSoundManager,
	std::weak_ptr<Player> pPlayer) :
	Scene(controller),
	m_pSoundManager(pSoundManager)
#ifdef _DEBUG
	, m_pPlayer(pPlayer)
#endif
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

#ifdef _DEBUG
	//プレイヤー位置ワープ画面を開いている間は、選択肢一覧の操作を止めて専用処理に任せる
	if (m_isPlayerWarpMode)
	{
		UpdatePlayerWarpMode();
		return;
	}
#endif

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
#ifdef _DEBUG
		case Select::ToggleDebugDraw:
		{
			//デバッグ表示のON/OFFを切り替える(シーン遷移はしない)
			DebugManager::GetInstance().ToggleDebugDrawEnabled();
			break;
		}
		case Select::Restart:
		{
			//ゲームを最初からやり直す
			m_controller.ChangeScene(
				std::make_shared<GameScene>(
					m_controller), 0.0f);
			break;
		}
		case Select::WarpPlayer:
		{
			//プレイヤー位置ワープ画面を開く
			m_isPlayerWarpMode = true;
			break;
		}
#endif
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

#ifdef _DEBUG
void PauseScene::UpdatePlayerWarpMode()
{
	InputManager& input = InputManager::GetInstance();
	auto pSound = m_pSoundManager.lock();

	//左スティックの左右でカーソルを移動させる
	int stickX = input.GetBufX();
	if (std::abs(stickX) > warp_stick_threshold)
	{
		m_warpCursorRatio += (static_cast<float>(stickX) / 1000.0f) * warp_cursor_move_speed;
		m_warpCursorRatio = std::clamp(m_warpCursorRatio, 0.0f, 1.0f);
	}

	//Aボタンで決定し、プレイヤーをワープさせる
	if (input.IsTriggered(InputEvent::ok))
	{
		if (pSound) pSound->Play(SoundManager::SoundType::Decision);

		auto pPlayer = m_pPlayer.lock();
		if (pPlayer)
		{
			Vector3 pos = pPlayer->GetPos();
			pos.z = warp_range_min_z +
				(warp_range_max_z - warp_range_min_z) * m_warpCursorRatio;
			pPlayer->SetPos(pos);
		}

		//ワープ画面とポーズ画面の両方を閉じてゲームに戻る
		m_isPlayerWarpMode = false;
		m_controller.PopScene();
	}
	//Bボタンでワープをキャンセルして選択肢一覧に戻る
	else if (input.IsTriggered(InputEvent::close))
	{
		if (pSound) pSound->Play(SoundManager::SoundType::Decision);
		m_isPlayerWarpMode = false;
	}
}
#endif

void PauseScene::Draw()
{
	const auto& wsize = Application::GetInstance().GetWindowSize();
	//UIの見た目の大きさをDebug/Releaseで揃えるためのスケール
	float uiScale = Application::GetInstance().GetUIScale();

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, black_graph_alpha);
	DrawBox(0, 0, wsize.width, wsize.height, 0x000000, true);
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
		wsize.width * back_ground_ratio_x, wsize.height * back_ground_ratio_y,
		back_ground_graph_scale * uiScale, backgroundH,
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
			wsize.width * back_game_ratio.x,
			wsize.height * back_game_ratio.y,
			select_graph_scale * uiScale, backGameHandle,
			1.0f,
			uvMaxU,
			uvMinU
		);
		//タイトルに戻る選択肢を描画
		DrawGraphToShaderByCenter(
			wsize.width * back_title_ratio.x,
			wsize.height * back_title_ratio.y,
			select_graph_scale * uiScale, backTitleHandle,
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
					wsize.width * back_game_ratio.x,
					wsize.height * back_game_ratio.y,
					select_graph_scale * uiScale, backGameOnCursorHandle,
					1.0f,
					m_wipeProgress[static_cast<int>(Select::BackGame)]
				);
			}
			DrawGraphToShaderByCenter(
				wsize.width * back_title_ratio.x,
				wsize.height * back_title_ratio.y,
				select_graph_scale * uiScale, backTitleHandle,
				1.0f
			);
			break;
		}
		case Select::BackTitle:
		{
			if (m_wipeProgress[static_cast<int>(Select::BackTitle)] > 0.0f)
			{
				DrawGraphToShaderByCenter(
					wsize.width * back_title_ratio.x,
					wsize.height * back_title_ratio.y,
					select_graph_scale * uiScale, backTitleOnCursorHandle,
					1.0f, m_wipeProgress[static_cast<int>(Select::BackTitle)]
				);
			}
			DrawGraphToShaderByCenter(
				wsize.width * back_game_ratio.x,
				wsize.height * back_game_ratio.y,
				select_graph_scale * uiScale, backGameHandle,
				1.0f
			);
			break;
		}
		}
	}

#ifdef _DEBUG
	//デバッグ表示ON/OFFの選択肢(画像を持たないため文字で表示する)
	bool isDebugDrawEnabled = DebugManager::GetInstance().IsDebugDrawEnabled();
	unsigned int debugSelectColor =
		(m_select == Select::ToggleDebugDraw) ? 0xffff00 : 0xffffff;
	DrawFormatString(
		static_cast<int>(wsize.width * debug_toggle_ratio.x),
		static_cast<int>(wsize.height * debug_toggle_ratio.y),
		debugSelectColor,
		L"デバッグ表示 : %s",
		isDebugDrawEnabled ? L"ON" : L"OFF"
	);

	//最初からやり直す選択肢(画像を持たないため文字で表示する)
	unsigned int restartSelectColor =
		(m_select == Select::Restart) ? 0xffff00 : 0xffffff;
	DrawFormatString(
		static_cast<int>(wsize.width * restart_ratio.x),
		static_cast<int>(wsize.height * restart_ratio.y),
		restartSelectColor,
		L"最初からやり直す"
	);

	//プレイヤーを移動選択肢(画像を持たないため文字で表示する)
	unsigned int warpPlayerSelectColor =
		(m_select == Select::WarpPlayer) ? 0xffff00 : 0xffffff;
	DrawFormatString(
		static_cast<int>(wsize.width * warp_player_ratio.x),
		static_cast<int>(wsize.height * warp_player_ratio.y),
		warpPlayerSelectColor,
		L"プレイヤーを移動"
	);

	//プレイヤー位置ワープ画面を開いている間は、物差しUIを上から重ねて表示する
	if (m_isPlayerWarpMode)
	{
		DrawPlayerWarpMode();
	}
#endif

	SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, ShaderRegister::glitch_buffer);
	SetUsePixelShader(-1);
}

#ifdef _DEBUG
void PauseScene::DrawPlayerWarpMode()
{
	const auto& wsize = Application::GetInstance().GetWindowSize();

	//物差しの背景を少し暗くして見やすくする
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
	DrawBox(0, 0, wsize.width, wsize.height, 0x000000, true);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	int rulerLeftX = static_cast<int>(wsize.width * warp_ruler_ratio_left);
	int rulerRightX = static_cast<int>(wsize.width * warp_ruler_ratio_right);
	int rulerY = static_cast<int>(wsize.height * warp_ruler_ratio_y);

	//物差し本体(横一本の線)
	DrawLine(rulerLeftX, rulerY, rulerRightX, rulerY, 0xffffff, 3);

	//左端に初期位置の目盛りとラベル
	DrawLine(rulerLeftX, rulerY - 15, rulerLeftX, rulerY + 15, 0xffffff, 3);
	DrawFormatString(rulerLeftX - 40, rulerY + 25, 0xffffff, L"初期位置");

	//右端にボスの目盛りとラベル
	DrawLine(rulerRightX, rulerY - 15, rulerRightX, rulerY + 15, 0xffffff, 3);
	DrawFormatString(rulerRightX - 20, rulerY + 25, 0xffffff, L"ボス");

	//現在選んでいる位置にカーソル(三角)を表示する
	int cursorX = rulerLeftX +
		static_cast<int>((rulerRightX - rulerLeftX) * m_warpCursorRatio);
	DrawTriangle(
		cursorX - 10, rulerY - 30,
		cursorX + 10, rulerY - 30,
		cursorX, rulerY - 10,
		0xffff00, true);

	//現在選んでいる位置のZ座標を数値でも表示する
	float targetZ = warp_range_min_z +
		(warp_range_max_z - warp_range_min_z) * m_warpCursorRatio;
	DrawFormatString(
		cursorX - 40, rulerY - 60,
		0xffff00,
		L"Z : %.0f", targetZ);

	//操作説明
	DrawFormatString(
		rulerLeftX, rulerY + 70,
		0xffffff,
		L"左スティック:移動　Aボタン:決定　Bボタン:キャンセル");
}
#endif

SceneID PauseScene::GetSceneID() const
{
	return SceneID::Pause;
}
