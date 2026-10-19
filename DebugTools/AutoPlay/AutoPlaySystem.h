#pragma once
#ifdef _DEBUG
#include <memory>
#include <string>

class AutoPlayWorld;
class AutoPlayBot;
class PlaytestRecorder;

/// <summary>
/// AIテストプレイの本体(Debugビルド専用)
/// 起動引数に--autoplayがあるときだけ動き、ゲームのコードは一切変えずに
/// DxLibの関数(パッド入力・時間・画面のフリップ・サウンド再生)の呼び出し先を差し替えて割り込む
/// </summary>
class AutoPlaySystem
{
public:
	//画面の出し方
	enum class Mode
	{
		Visible,//ゲーム画面を表示する
		Hidden,//ウィンドウを隠して進捗だけ記録する
	};

	//起動引数で決める設定
	struct Settings
	{
		bool isEnabled = false;
		int runCount = 10;//何回プレイするか
		int speed = 1;//何倍速か(0は最大速度)
		Mode mode = Mode::Visible;
		std::wstring skillName = L"normal";
		unsigned int seed = 0;
		std::wstring outputDir = L"DebugTools/AutoPlay/Logs";
		int maxMinutes = 15;//1回のプレイの上限(ゲーム内の時間、分)
		bool isMute = true;
	};

	static AutoPlaySystem& GetInstance();
	~AutoPlaySystem();

	//起動時(WinMainより前)に1回呼ばれる。--autoplayがあれば関数を差し替える
	void Boot();

	//以下は差し替えた関数から呼ばれる
	//毎フレームの入力の直前:AIの入力を決める
	void OnInputFrame(unsigned char* buttons, short& thumbLX, short& thumbLY);
	//画面のフリップの直前:進捗を描き、実際にフリップするかを返す
	bool OnScreenFlip();
	//時間を何倍に進めるか(0は最大速度)
	int GetSpeed() const { return m_settings.speed; }
	//画面を出さないモードか
	bool IsHidden() const { return m_settings.mode == Mode::Hidden; }

private:
	AutoPlaySystem();
	AutoPlaySystem(const AutoPlaySystem&) = delete;
	void operator=(const AutoPlaySystem&) = delete;

	//起動引数を読む
	void ParseCommandLine();
	//関数を差し替える
	void InstallHooks();
	//1回のプレイを終える
	void FinishRun(const char* result);
	//進捗の表示を描く
	void DrawOverlay();

private:
	Settings m_settings;
	std::wstring m_hookReport;//関数の差し替えの結果(表示用)

	std::unique_ptr<AutoPlayWorld> m_pWorld;
	std::unique_ptr<AutoPlayBot> m_pBot;
	std::unique_ptr<PlaytestRecorder> m_pRecorder;

	bool m_isFirstFrame = true;
	bool m_isFinished = false;
	int m_frame = 0;
	int m_runIndex = 0;//今のプレイの番号(1から)
	int m_menuFrame = 0;//メニュー画面にいる時間
	int m_tutorialSkipFrame = 0;//チュートリアルを飛ばす操作の経過
	int m_restartFrame = -1;//時間切れのやり直し操作の経過(-1:していない)
	bool m_wasPlayerSeen = false;//前フレームにプレイヤーがいたか
};
#endif
