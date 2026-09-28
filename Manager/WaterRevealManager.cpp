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
    //ブレンドを無効化してシェーダー出力のアルファをそのまま書き込む
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 255);
    //キャプチャ開始フラグを立てる
    m_isCapture = true;
}

void WaterRevealManager::EndCapture()
{
    //描画先をバックバッファに戻す
    SetDrawScreen(DX_SCREEN_BACK);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 255);
    //キャプチャフラグを降ろす
    m_isCapture = false;
    MV1SetUseOrigShader(false);
    SetUseVertexShader(-1);
    SetUsePixelShader(-1);
}

void WaterRevealManager::ClearCapture()
{
    //キャプチャ画面を描画先にして中身をクリア(アルファ込みで透明になる)
    SetDrawScreen(m_captureH);
    ClearDrawScreen();
    //描画先をバックバッファに戻す
    SetDrawScreen(DX_SCREEN_BACK);
}
