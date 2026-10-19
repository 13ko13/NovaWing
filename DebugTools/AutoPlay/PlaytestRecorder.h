#pragma once
#ifdef _DEBUG
#include <string>
#include <vector>

class AutoPlayWorld;

/// <summary>
/// AIテストプレイ用:プレイ中の出来事(被弾・死亡・カウンターなど)を記録し、JSONに書き出す
/// (プレイヤーの各種カウンターを毎フレーム見比べて出来事を見つける。ゲーム側には何も書き込まない)
/// </summary>
class PlaytestRecorder
{
public:
	//テスト全体の設定(ログに一緒に残す)
	struct SessionInfo
	{
		std::wstring skillName;
		std::wstring speedText;
		std::wstring modeText;
		int runCount = 0;
		unsigned int seed = 0;
	};

	PlaytestRecorder();
	virtual ~PlaytestRecorder();

	//テストの最初に呼ぶ
	void BeginSession(const SessionInfo& info, const std::wstring& outputDir);

	//1回のプレイの開始・更新・終了
	void BeginRun(int runIndex);
	void UpdateRun(const AutoPlayWorld& world);
	//result:clear/death/timeout/aborted
	void EndRun(const char* result);

	//プレイ中か
	bool IsRunActive() const { return m_isRunActive; }
	//今のプレイの結果が出たか(死亡・クリア)
	const char* GetDecidedResult() const { return m_decidedResult; }
	//通常ステージが始まっているか
	bool IsStageStarted() const { return m_isStageStarted; }
	//今のプレイの経過フレーム
	int GetRunFrame() const { return m_runFrame; }

	//画面表示・進捗表示用
	float GetStageZ() const { return m_lastStageZ; }
	int GetHealth() const { return m_prevHealth; }
	int GetFinishedRunCount() const { return static_cast<int>(m_runs.size()); }
	int GetClearCount() const;

	//進捗を書き出す(画面を出さないモードで、起動スクリプトが読む)
	void WriteStatus(int runIndex, int runCount, bool isFinished) const;

private:
	//出来事1つ
	struct Event
	{
		std::string type;
		int frame = 0;
		float z = 0.0f;
		float x = 0.0f;
		float y = 0.0f;
		std::string cause;
		int value = 0;
		int health = 0;
		bool isBossPhase = false;//ボス戦中の出来事か
	};
	//一定間隔で取る状態
	struct Sample
	{
		int frame = 0;
		float z = 0.0f;
		int health = 0;
		float gauge = 0.0f;
	};
	//1回のプレイの記録
	struct Run
	{
		int index = 0;
		std::string result;
		int frames = 0;
		int stageStartFrame = -1;
		int bossAppearFrame = -1;
		float reachedZ = 0.0f;
		std::string deathCause;
		std::vector<Event> events;
		std::vector<Sample> samples;
	};
	//ステージ構成(最初のプレイで1回だけ記録)
	struct LayoutItem
	{
		std::string type;
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		float radius = 0.0f;
	};

	//被弾の原因を、プレイヤーの近くにあるものから推測する
	std::string GuessDamageCause(const AutoPlayWorld& world) const;
	//出来事を追加する
	void AddEvent(const AutoPlayWorld& world, const char* type, const std::string& cause = "", int value = 0);
	//全てのプレイをJSONに書き出す
	void WriteFiles() const;
	//JSONの文字列を作る
	std::string BuildJson() const;

private:
	SessionInfo m_info;
	std::wstring m_outputDir;
	std::wstring m_sessionFileName;
	std::string m_startedAt;

	std::vector<Run> m_runs;
	Run m_currentRun;
	bool m_isRunActive = false;
	const char* m_decidedResult = nullptr;

	std::vector<LayoutItem> m_layout;
	bool m_isLayoutRecorded = false;

	//通常ステージ
	bool m_isStageStarted = false;
	float m_stageOffsetZ = 0.0f;
	float m_lastStageZ = 0.0f;
	int m_runFrame = 0;

	//前フレームのプレイヤーの状態(差を見て出来事を見つける)
	int m_prevHitCount = 0;
	int m_prevHealth = 0;
	int m_prevCounterCount = 0;
	int m_prevDefeatedCount = 0;
	int m_prevChargeDefeatedCount = 0;
	bool m_wasRolling = false;
	bool m_wasInvincible = false;
	bool m_wasBoosting = false;
	bool m_wasBraking = false;
	bool m_wasSomersault = false;
	bool m_wasDisabled = false;
	bool m_wasBossDying = false;
	bool m_isPlayerSeen = false;
	bool m_isBossPhase = false;
	std::string m_lastDamageCause;
};
#endif
