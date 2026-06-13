#include <memory>
#include <cassert>
#include <DxLib.h>

#include "Application.h"
#include "../Manager/InputManager.h"
#include "../Scene/SceneController.h"
#include "../Constants/Game.h"
#include "../Manager/ResourceLoader.h"

namespace
{
	
}

Application::Application():
	m_windowSize{Game::screen_width,Game::screen_height}
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
	ChangeWindowMode(false);
	//ゲーム名
	SetMainWindowText(L"NovaWing");
	//画面サイズと色数を設定
	SetGraphMode(m_windowSize.m_width, m_windowSize.m_height, Game::color_bit_num);

	//エフェクトを描画するためにDirect3D11を使用するように設定（DxLib_Initの前に呼ぶ必要がある）
	SetUseDirect3DVersion(DX_DIRECT3D_11);

	if (DxLib_Init() == -1)
	{
		return false;
	}

	//Effekseer_Initの後に呼ばないとエフェクトロードに失敗する
	//リソースローダーのインスタンスを作成して
	//リソースをロードする
	ResourceLoader::GetInstance().LoadAll();

	// Zバッファを有効にする。
	SetUseZBuffer3D(TRUE);
	// Zバッファへの書き込みを有効にする。
	SetWriteZBuffer3D(TRUE);

	return true;
}

void Application::Run()
{
	SetDrawScreen(DX_SCREEN_BACK);
	InputManager input;//入力のためのオブジェクト 
	SceneController controller;//シーンを管理するオブジェクト
	//TODO:GameSceneを作成してChangeSceneで読み込む

	while (ProcessMessage() != -1)
	{
		auto startTime = GetNowHiPerformanceCount();

		ClearDrawScreen();
		input.Update();//入力状態の更新

		//シーンの更新
		controller.Update(input);
		//シーンの描画
		controller.Draw();
		ScreenFlip();
		//ESCキーが押されたらウィンドウを閉じる
		if (CheckHitKey(KEY_INPUT_ESCAPE))
		{
			break;
		}

		while (GetNowHiPerformanceCount() - startTime < 16667)
		{

		}
	}
}

void Application::Terminate()
{
	//リソースを解放する
	ResourceLoader::GetInstance().ReleaseAll();
	DxLib_End();
}

const Size& Application::GetWindowSize() const
{
	return m_windowSize;
}
