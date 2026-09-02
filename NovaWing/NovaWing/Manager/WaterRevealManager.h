#pragma once

//オブジェクト周りにある水の透明化に使用する
class WaterRevealManager
{
public:
    /// <summary>
	/// WaterRevealManagerのインスタンスを取得する
	/// </summary>
	/// <returns></returns>
	static WaterRevealManager& GetInstance();

	//コピーと代入を禁止する(消す)
	WaterRevealManager(const WaterRevealManager&) = delete;
    WaterRevealManager& operator=(const WaterRevealManager&) = delete;

    //キャプチャ画面に描画先を切り替える
    void BeginCapture();

    //バックバッファに描画先を戻す
    void EndCapture();

    //キャプチャ画面の中身を空(透明)にする
    //キャプチャパスを回さないシーン(タイトル等)に入るとき、前のシーンの描画内容が
    //残ってWaterManagerに透けて見えるのを防ぐために1度だけ呼ぶ
    void ClearCapture();

    //キャプチャ中かどうかを返す
    bool IsCaptureMode() const { return m_isCapture; }

    //キャプチャ画面のハンドルを返す(WaterManagerに参照される)
    int GetCaptureTextureHandle() const { return m_captureH; }

    //CapturePS.psoのハンドルをかえす
    int GetCapturePSHandle() const { return m_capturePSH; }

private:
    //コンストラクタ
    WaterRevealManager();

private:
    //キャプチャー画面のハンドル
    int m_captureH = -1;
    //CapturePSのハンドル
    int m_capturePSH = -1;
    //キャプチャ中か
    bool m_isCapture = false;
};

