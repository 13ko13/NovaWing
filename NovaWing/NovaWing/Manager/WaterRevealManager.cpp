#include <DxLib.h>

#include "WaterRevealManager.h"
#include "Constants/Game.h"

WaterRevealManager::WaterRevealManager()
{
    //描画先を作る
    m_captureH = MakeScreen(Game::screen_width, Game::screen_height,true);
    //キャプチャに使用するピクセルシェーダのロード
    m_capturePSH = LoadPixelShader(L"CapturePS.pso");
}

WaterRevealManager& WaterRevealManager::GetInstance()
{
    //宣言してインスタンスを作成して返す
    static WaterRevealManager instance;
    return instance;
}

void WaterRevealManager::BeginCapture()
{
    //描画先を切り替える
    SetDrawScreen(m_captureH);
    //前フレームの描画内容をクリア
    ClearDrawScreen();
    //キャプチャ開始フラグを立てる
    m_isCapture = true;
}

void WaterRevealManager::EndCapture()
{
    //描画先をバックバッファに戻す
    SetDrawScreen(DX_SCREEN_BACK);
    //キャプチャフラグを降ろす
    m_isCapture = false;
}
