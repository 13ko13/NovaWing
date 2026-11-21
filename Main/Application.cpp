#define NOMINMAX
#include <memory>
#include <cassert>
#include <DxLib.h>
#include <EffekseerForDXLib.h>

#include "Application.h"
#include "../Manager/InputManager.h"
#include "../Manager/DebugManager.h"
#include "../Scene/SceneController.h"
#include "../Constants/Game.h"
#include "../Manager/ResourceLoader.h"
#include "../Scene/TitleScene.h"
#ifdef _DEBUG
#include "../Scene/GameScene.h"
#include "../Scene/GameoverScene.h"
#include "../Scene/ClearScene.h"
#include "../DebugTools/DebugUI/DebugUI.h"
#endif

namespace
{
#ifdef _DEBUG
	//デバッグのシーン移動でのフェードの長さ
	constexpr float debug_scene_change_fade_frame = 30.0f;
	//ステージ番号(1面=0)
	constexpr int stage1_no = 0;
	constexpr int stage2_no = 1;

	/// <summary>
	/// どのシーンからでも、デバッグ用ウィンドウでシーンを移動できるようにする
	/// </summary>
	/// <param name="controller">シーンを管理するオブジェクト(Runが終わるまで生きている物)</param>
	void RegisterSceneDebugItems(SceneController& controller)
	{
		DebugUI& debugUI = DebugUI::GetInstance();
		//登録者の目印にはcontrollerのアドレスを使う
		const void* owner = &controller;

		debugUI.AddButton(owner, L"Scene", L"タイトル", [&controller]()
			{
				controller.ChangeScene(
					std::make_shared<TitleScene>(controller), debug_scene_change_fade_frame);
			});
		debugUI.AddButton(owner, L"Scene", L"1面", [&controller]()
			{
				controller.ChangeScene(
					std::make_shared<GameScene>(controller, stage1_no), debug_scene_change_fade_frame);
			});
		debugUI.AddButton(owner, L"Scene", L"2面", [&controller]()
			{
				controller.ChangeScene(
					std::make_shared<GameScene>(controller, stage2_no), debug_scene_change_fade_frame);
			});
		debugUI.AddButton(owner, L"Scene", L"ゲームオーバー(1面)", [&controller]()
			{
				controller.ChangeScene(
					std::make_shared<GameoverScene>(controller, stage1_no), debug_scene_change_fade_frame);
			});
		debugUI.AddButton(owner, L"Scene", L"クリア(1面・仮の結果)", [&controller]()
			{
				//リザルトの数字は見た目の確認用の仮の値
				ClearScene::ClearResultData data;
				data.defeatedEnemyCount = 30;
				data.clearTime = 60 * 120;
				data.hitCount = 5;
				data.stageNo = stage1_no;
				controller.ChangeScene(
					std::make_shared<ClearScene>(controller, data), debug_scene_change_fade_frame);
			});
	}
#endif
}

Application::Application() :
	m_windowSize{ Game::screen_width,Game::screen_height }
{

}

Application::~Application()
{

}

Application& Application::GetInstance()
{
	//staticで宣言してそれを返す
	static Application instance;
	return instance;
}

bool Application::Init()
{
	//ウィンドウモード設定
#ifdef _DEBUG
	//デバッグ中はウィンドウモードにする
	ChangeWindowMode(true);
#else
	//リリース中はフルスクリーンにする
	ChangeWindowMode(false);
#endif
	//ゲーム名
	SetMainWindowText(L"NovaWing");
	//画面サイズと色数を設定
	SetGraphMode(m_windowSize.width, m_windowSize.height, Game::color_bit_num);

	//エフェクトを描画するためにDirect3D11を使用するように設定（DxLib_Initの前に呼ぶ必要がある）
	SetUseDirect3DVersion(DX_DIRECT3D_11);

	if (DxLib_Init() == -1)
	{
		return false;
	}
	if (Effekseer_Init(8000) == -1)
	{
		return false;
	}
#ifdef _DEBUG
	//デバッグ用ウィンドウ(Dear ImGui)を初期化
	if (!DebugUI::GetInstance().Init())
	{
		return false;
	}
#endif

	// Zバッファを有効にする。
	SetUseZBuffer3D(TRUE);
	// Zバッファへの書き込みを有効にする。
	SetWriteZBuffer3D(TRUE);

	return true;
}

void Application::Run()
{
	SetDrawScreen(DX_SCREEN_BACK);
	SceneController controller;//シーンを管理するオブジェクト

	//TitleSceneを作成してChangeSceneで読み込む
	controller.ChangeScene(
		std::make_shared<TitleScene>(
			controller), 60.0f);

#ifdef _DEBUG
	//デバッグ用ウィンドウにシーン移動のボタンを登録する
	RegisterSceneDebugItems(controller);
#endif

	while (ProcessMessage() != -1)
	{
		auto startTime = GetNowHiPerformanceCount();

		ClearDrawScreen();
#ifdef _DEBUG
		//デバッグ用ウィンドウの組み立て(入力の取得より先に行い、文字入力中かどうかを入力側へ反映させる)
		DebugUI::GetInstance().BeginFrame();
#endif
		InputManager::GetInstance().Update();

		//シーンの更新
		controller.Update();
		//シーンの描画
		controller.Draw();

#ifdef _DEBUG
		//デバッグ用ウィンドウをゲーム画面の上に描画
		DebugUI::GetInstance().Render();
#endif

		ScreenFlip();
		//ESCキーが押されたらウィンドウを閉じる
		if (CheckHitKey(KEY_INPUT_ESCAPE) ||
			m_isExitRequested)
		{
			break;
		}

		while (GetNowHiPerformanceCount() - startTime < 16667)
		{

		}
	}

#ifdef _DEBUG
	//controllerが消える前に、ボタンの登録を解除する
	DebugUI::GetInstance().RemoveOwner(&controller);
#endif
}

void Application::Terminate()
{
	//リソースを解放する
	ResourceLoader::GetInstance().ReleaseAll();
#ifdef _DEBUG
	DebugUI::GetInstance().Terminate();
#endif
	//先にEffekseerを終了する
	Effkseer_End();
	DxLib_End();
}

const Size& Application::GetWindowSize() const
{
	return m_windowSize;
}

float Application::GetUIScale() const
{
	//基準解像度に対する実際のウィンドウ幅の比率をUIスケールとする
	return static_cast<float>(m_windowSize.width) / static_cast<float>(Game::base_screen_width);
}

void Application::StartSlow(float scale, int frame)
{
	//タイムスケールとスローの時間を設定
	//すでにスロー中であれば残りフレームが長い方を設定
	m_timeScale = scale;
	m_slowFrame = std::max(m_slowFrame, frame);
}

void Application::UpdateTimeScale()
{
	if (m_slowFrame > 0)
	{
		m_slowFrame--;
		//0になればタイムスケールを通常に戻す
		if (m_slowFrame == 0)
		{
			m_timeScale = 1.0f;
		}
	}
}

void Application::ResetTimeScale()
{
	//タイムスケールをリセットさせる
	m_timeScale = 1.0f;
	m_slowFrame = 0;
}
