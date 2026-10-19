#include "AutoPlaySystem.h"
#ifdef _DEBUG
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <string>
#include <vector>

#include <DxLib.h>
#include <shellapi.h>

#include "AutoPlayWorld.h"
#include "AutoPlayBot.h"
#include "PlaytestRecorder.h"
#include "Main/Application.h"

namespace
{
	//1フレームの長さ(マイクロ秒、Application::Runの待ちと同じ値)
	constexpr LONGLONG frame_micro_sec = 16667;
	//最大速度で画面を出すときに、フリップする間隔(マイクロ秒)
	constexpr LONGLONG max_speed_flip_micro_sec = 33000;

	//メニューで決定ボタンを押す間隔
	constexpr int menu_press_interval = 30;
	//進捗ファイルを書き出す間隔
	constexpr int status_write_interval = 60;
	//XInputのスティックの最大値
	constexpr float thumb_max = 32767.0f;

	//チュートリアルを飛ばす操作(ポーズ→下4回で「プレイヤーを移動」→決定→物差しの左端で決定)
	constexpr int skip_pause_frame = 5;
	constexpr int skip_first_down_frame = 8;
	constexpr int skip_down_count = 4;
	constexpr int skip_down_interval = 3;
	constexpr int skip_select_frame = 30;
	constexpr int skip_warp_frame = 36;
	//うまくいかなかったとき(チュートリアルのメッセージが出ていたなど)にやり直すまでのフレーム
	constexpr int skip_retry_frame = 90;
	//時間切れのやり直し(ポーズ→下3回で「最初からやり直す」→決定)
	constexpr int restart_down_count = 3;

	//------------------------------------------------------------
	//DxLibの関数の差し替え
	//------------------------------------------------------------
	//Debugビルドは関数呼び出しが「ILT」というジャンプ命令の中継を通るので、その飛び先だけを書き換える
	//戻り値は元の関数の場所(ILTでなかったときはnullptr)
	void* RedirectFunction(void* target, void* hook, bool& isSucceeded)
	{
		isSucceeded = false;
		uint8_t* code = static_cast<uint8_t*>(target);
		constexpr uint8_t jmp_rel32 = 0xE9;
		constexpr int jmp_size = 5;
		if (code[0] != jmp_rel32) return nullptr;

		int32_t oldRel = 0;
		std::memcpy(&oldRel, code + 1, sizeof(oldRel));
		uint8_t* original = code + jmp_size + oldRel;

		intptr_t newRel = static_cast<uint8_t*>(hook) - (code + jmp_size);
		if (newRel > INT32_MAX || newRel < INT32_MIN) return nullptr;
		int32_t rel32 = static_cast<int32_t>(newRel);

		DWORD oldProtect = 0;
		if (!VirtualProtect(code + 1, sizeof(rel32), PAGE_EXECUTE_READWRITE, &oldProtect)) return nullptr;
		std::memcpy(code + 1, &rel32, sizeof(rel32));
		VirtualProtect(code + 1, sizeof(rel32), oldProtect, &oldProtect);
		FlushInstructionCache(GetCurrentProcess(), code, jmp_size);

		isSucceeded = true;
		return original;
	}

	using ScreenFlipFunc = int (*)();
	ScreenFlipFunc g_originalScreenFlip = nullptr;

	//本当の時間(マイクロ秒)
	LONGLONG RealMicroSec()
	{
		static LARGE_INTEGER frequency = {};
		if (frequency.QuadPart == 0) QueryPerformanceFrequency(&frequency);
		LARGE_INTEGER counter;
		QueryPerformanceCounter(&counter);
		return counter.QuadPart / frequency.QuadPart * 1000000 +
			counter.QuadPart % frequency.QuadPart * 1000000 / frequency.QuadPart;
	}

	//パッド入力:実際のパッドの代わりにAIの入力を返す
	int HookedGetJoypadXInputState(int inputType, XINPUT_STATE* pState)
	{
		if (!pState) return -1;
		*pState = XINPUT_STATE();
		if (inputType != DX_INPUT_PAD1) return -1;
		AutoPlaySystem::GetInstance().OnInputFrame(pState->Buttons, pState->ThumbLX, pState->ThumbLY);
		return 0;
	}

	//時間:指定の倍率で進める(Application::Runの1フレームの待ちが短くなる)
	LONGLONG HookedGetNowHiPerformanceCount(int)
	{
		static LONGLONG realBase = RealMicroSec();
		static LONGLONG virtualNow = realBase;

		int speed = AutoPlaySystem::GetInstance().GetSpeed();
		if (speed <= 0)
		{
			//最大速度:呼ばれるたびに1フレーム分進め、待ちをなくす
			virtualNow += frame_micro_sec;
			return virtualNow;
		}
		LONGLONG now = realBase + (RealMicroSec() - realBase) * speed;
		if (now > virtualNow) virtualNow = now;
		return virtualNow;
	}

	//画面のフリップ:倍速中は何フレームかに1回だけ表示する
	int HookedScreenFlip()
	{
		bool isFlip = AutoPlaySystem::GetInstance().OnScreenFlip();
		if (isFlip && g_originalScreenFlip) return g_originalScreenFlip();
		return 0;
	}

	//サウンド再生:音を消す設定のときは鳴らさない
	int HookedPlaySoundMem(int, int, int)
	{
		return 0;
	}

	//起動時に一度だけAIテストプレイの準備をする(WinMainより前に動く)
	struct Bootstrap
	{
		Bootstrap()
		{
			AutoPlaySystem::GetInstance().Boot();
		}
	};
	Bootstrap g_bootstrap;

	//"--name=value"形式の起動引数を探す
	bool FindArgument(const std::vector<std::wstring>& args, const std::wstring& name, std::wstring& outValue)
	{
		std::wstring prefix = L"--" + name;
		for (const std::wstring& arg : args)
		{
			if (arg == prefix)
			{
				outValue.clear();
				return true;
			}
			if (arg.rfind(prefix + L"=", 0) == 0)
			{
				outValue = arg.substr(prefix.size() + 1);
				return true;
			}
		}
		return false;
	}
}

AutoPlaySystem& AutoPlaySystem::GetInstance()
{
	static AutoPlaySystem instance;
	return instance;
}

AutoPlaySystem::AutoPlaySystem() :
	m_pWorld(std::make_unique<AutoPlayWorld>()),
	m_pBot(std::make_unique<AutoPlayBot>()),
	m_pRecorder(std::make_unique<PlaytestRecorder>())
{
}

AutoPlaySystem::~AutoPlaySystem()
{
}

void AutoPlaySystem::Boot()
{
	ParseCommandLine();
	if (!m_settings.isEnabled) return;
	InstallHooks();
}

void AutoPlaySystem::ParseCommandLine()
{
	int argCount = 0;
	LPWSTR* argList = CommandLineToArgvW(GetCommandLineW(), &argCount);
	if (!argList) return;
	std::vector<std::wstring> args(argList, argList + argCount);
	LocalFree(argList);

	std::wstring value;
	if (!FindArgument(args, L"autoplay", value)) return;
	m_settings.isEnabled = true;

	if (FindArgument(args, L"runs", value)) m_settings.runCount = (std::max)(1, _wtoi(value.c_str()));
	if (FindArgument(args, L"speed", value))
	{
		m_settings.speed = (value == L"max") ? 0 : (std::max)(1, _wtoi(value.c_str()));
	}
	if (FindArgument(args, L"mode", value) && value == L"hidden") m_settings.mode = Mode::Hidden;
	if (FindArgument(args, L"skill", value)) m_settings.skillName = value;
	if (FindArgument(args, L"seed", value))
	{
		m_settings.seed = static_cast<unsigned int>(_wtoi(value.c_str()));
	}
	else
	{
		m_settings.seed = static_cast<unsigned int>(GetTickCount64());
	}
	if (FindArgument(args, L"out", value) && !value.empty()) m_settings.outputDir = value;
	if (FindArgument(args, L"max-minutes", value)) m_settings.maxMinutes = (std::max)(1, _wtoi(value.c_str()));

	//音は等速で画面を出すときだけ鳴らす(--sound/--muteで指定もできる)
	m_settings.isMute = !(m_settings.speed == 1 && m_settings.mode == Mode::Visible);
	if (FindArgument(args, L"sound", value)) m_settings.isMute = false;
	if (FindArgument(args, L"mute", value)) m_settings.isMute = true;

	//知らない腕前ならnormalにする
	AutoPlayBot::Skill skill;
	if (!AutoPlayBot::FindSkill(m_settings.skillName, skill)) m_settings.skillName = L"normal";
}

void AutoPlaySystem::InstallHooks()
{
	bool isSucceeded = false;
	m_hookReport.clear();

	RedirectFunction(reinterpret_cast<void*>(&GetJoypadXInputState),
		reinterpret_cast<void*>(&HookedGetJoypadXInputState), isSucceeded);
	m_hookReport += isSucceeded ? L"入力:OK " : L"入力:NG ";

	if (m_settings.speed != 1)
	{
		RedirectFunction(reinterpret_cast<void*>(&GetNowHiPerformanceCount),
			reinterpret_cast<void*>(&HookedGetNowHiPerformanceCount), isSucceeded);
		m_hookReport += isSucceeded ? L"速度:OK " : L"速度:NG ";
	}

	void* original = RedirectFunction(reinterpret_cast<void*>(&ScreenFlip),
		reinterpret_cast<void*>(&HookedScreenFlip), isSucceeded);
	g_originalScreenFlip = reinterpret_cast<ScreenFlipFunc>(original);
	m_hookReport += isSucceeded ? L"画面:OK " : L"画面:NG ";

	if (m_settings.isMute)
	{
		RedirectFunction(reinterpret_cast<void*>(&PlaySoundMem),
			reinterpret_cast<void*>(&HookedPlaySoundMem), isSucceeded);
		m_hookReport += isSucceeded ? L"消音:OK" : L"消音:NG";
	}
}

void AutoPlaySystem::OnInputFrame(unsigned char* buttons, short& thumbLX, short& thumbLY)
{
	//DxLibの初期化が終わった後の最初のフレームで、ウィンドウやテストの準備をする
	if (m_isFirstFrame)
	{
		m_isFirstFrame = false;
		//ウィンドウが裏に回っても止まらないようにする
		SetAlwaysRunFlag(TRUE);
		if (IsHidden())
		{
			SetWindowVisibleFlag(FALSE);
		}

		PlaytestRecorder::SessionInfo info;
		info.skillName = m_settings.skillName;
		info.speedText = (m_settings.speed <= 0) ? L"max" : L"x" + std::to_wstring(m_settings.speed);
		info.modeText = IsHidden() ? L"hidden" : L"visible";
		info.runCount = m_settings.runCount;
		info.seed = m_settings.seed;
		m_pRecorder->BeginSession(info, m_settings.outputDir);
	}

	m_frame++;
	AutoPlayBot::PadOutput pad;

	m_pWorld->Refresh();
	bool isPlayerSeen = m_pWorld->GetPlayer() != nullptr;

	if (m_isFinished)
	{
		//全てのプレイが終わったので何もしない(終了待ち)
	}
	else if (!isPlayerSeen)
	{
		//ゲームシーンから抜けたら、そのプレイは終わり
		if (m_wasPlayerSeen && m_pRecorder->IsRunActive())
		{
			FinishRun(nullptr);
		}

		//タイトル・ゲームオーバー・クリア画面では、ときどき決定ボタンを押す
		//(どの画面も最初の選択肢が「ゲーム開始」「リトライ」になっている)
		m_menuFrame++;
		pad.buttons[XINPUT_BUTTON_A] = (m_menuFrame % menu_press_interval == 0);
	}
	else
	{
		m_menuFrame = 0;

		//ゲームシーンに入ったら新しいプレイを始める
		if (!m_pRecorder->IsRunActive() && !m_wasPlayerSeen)
		{
			m_runIndex++;
			unsigned int runSeed = m_settings.seed + static_cast<unsigned int>(m_runIndex);
			AutoPlayBot::Skill skill;
			AutoPlayBot::FindSkill(m_settings.skillName, skill);
			m_pBot->Reset(skill, runSeed);
			m_pRecorder->BeginRun(m_runIndex);
			//ボスの攻撃の選び方などのゲーム側の乱数も、プレイごとに決まった値から始める
			SRand(static_cast<int>(runSeed));
			m_tutorialSkipFrame = 0;
			m_restartFrame = -1;
		}

		m_pRecorder->UpdateRun(*m_pWorld);

		if (m_restartFrame >= 0)
		{
			//時間切れ:ポーズから「最初からやり直す」を選ぶ
			int frame = m_restartFrame++;
			pad.buttons[XINPUT_BUTTON_START] = (frame == skip_pause_frame);
			for (int i = 0; i < restart_down_count; i++)
			{
				if (frame == skip_first_down_frame + i * skip_down_interval) pad.buttons[XINPUT_BUTTON_DPAD_DOWN] = true;
			}
			pad.buttons[XINPUT_BUTTON_A] = (frame == skip_select_frame);
			if (frame > skip_retry_frame) m_restartFrame = 0;
		}
		else if (m_pRecorder->IsRunActive() && !m_pRecorder->IsStageStarted())
		{
			//チュートリアル中:デバッグ用の「プレイヤーを移動」で通常ステージの最初へ飛ぶ
			int frame = m_tutorialSkipFrame++;
			pad.buttons[XINPUT_BUTTON_START] = (frame == skip_pause_frame);
			for (int i = 0; i < skip_down_count; i++)
			{
				if (frame == skip_first_down_frame + i * skip_down_interval) pad.buttons[XINPUT_BUTTON_DPAD_DOWN] = true;
			}
			pad.buttons[XINPUT_BUTTON_A] = (frame == skip_select_frame || frame == skip_warp_frame);
			if (frame > skip_retry_frame) m_tutorialSkipFrame = 0;
		}
		else if (m_pRecorder->IsRunActive() && !m_pRecorder->GetDecidedResult())
		{
			//時間切れならやり直す
			int maxFrame = m_settings.maxMinutes * 60 * 60;
			if (m_pRecorder->GetRunFrame() > maxFrame)
			{
				FinishRun("timeout");
				m_restartFrame = 0;
			}
			else
			{
				pad = m_pBot->Update(*m_pWorld);
			}
		}
	}
	m_wasPlayerSeen = isPlayerSeen;

	//進捗をときどき書き出す
	if (m_frame % status_write_interval == 0)
	{
		m_pRecorder->WriteStatus(m_runIndex, m_settings.runCount, m_isFinished);
	}

	//ゲームの終了をじゃましないよう、読み取ったオブジェクトはすぐ手放す
	m_pWorld->ReleaseObjects();

	//パッドの形に直す
	for (int i = 0; i < static_cast<int>(pad.buttons.size()); i++)
	{
		buttons[i] = pad.buttons[i] ? 1 : 0;
	}
	thumbLX = static_cast<short>(pad.stickX * thumb_max);
	thumbLY = static_cast<short>(pad.stickY * thumb_max);
}

void AutoPlaySystem::FinishRun(const char* result)
{
	m_pRecorder->EndRun(result);
	if (m_runIndex >= m_settings.runCount)
	{
		m_isFinished = true;
		m_pRecorder->WriteStatus(m_runIndex, m_settings.runCount, true);
		Application::GetInstance().RequestExit();
	}
}

bool AutoPlaySystem::OnScreenFlip()
{
	//画面を出さないモードではフリップしない
	if (IsHidden()) return false;

	DrawOverlay();

	if (m_settings.speed == 1) return true;

	if (m_settings.speed <= 0)
	{
		//最大速度:一定の時間ごとに表示する
		static LONGLONG lastFlip = 0;
		LONGLONG now = RealMicroSec();
		if (now - lastFlip < max_speed_flip_micro_sec) return false;
		lastFlip = now;
		return true;
	}

	//n倍速:nフレームに1回表示する(毎回フリップすると画面の更新待ちで速くならない)
	static int flipCount = 0;
	flipCount++;
	return flipCount % m_settings.speed == 0;
}

void AutoPlaySystem::DrawOverlay()
{
	constexpr int box_left = 8;
	constexpr int box_top = 8;
	constexpr int box_width = 470;
	constexpr int box_height = 92;
	constexpr int line_height = 20;
	constexpr int text_left = 16;
	constexpr int overlay_alpha = 170;

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, overlay_alpha);
	DrawBox(box_left, box_top, box_left + box_width, box_top + box_height, 0x000000, TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	std::wstring speedText = (m_settings.speed <= 0) ? L"最大" : L"x" + std::to_wstring(m_settings.speed);
	int y = box_top + 6;
	DrawFormatString(text_left, y, 0x60e9a4, L"AIテストプレイ  %d/%d回目  腕前:%s  速度:%s",
		m_runIndex, m_settings.runCount, m_settings.skillName.c_str(), speedText.c_str());
	y += line_height;
	DrawFormatString(text_left, y, 0xffffff, L"Z:%.0f  HP:%d  行動:%s",
		m_pRecorder->GetStageZ(), m_pRecorder->GetHealth(), m_pBot->GetActionName());
	y += line_height;
	DrawFormatString(text_left, y, 0xffffff, L"終了したプレイ:%d  クリア:%d",
		m_pRecorder->GetFinishedRunCount(), m_pRecorder->GetClearCount());
	y += line_height;
	DrawFormatString(text_left, y, 0xaaaaaa, L"ESCで中断  %s", m_hookReport.c_str());
}
#endif
