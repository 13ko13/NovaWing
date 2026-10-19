#include "PlaytestRecorder.h"
#ifdef _DEBUG
#include <cfloat>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <DxLib.h>

#include "AutoPlayWorld.h"
#include "Scene/GameScene.h"
#include "Game/GameObjects/Actors/Character/Player/Player.h"
#include "Game/GameObjects/Actors/Character/Enemy/BossEnemy/BossEnemy.h"

namespace
{
	//プレイヤーの当たり判定の中心のずれと半径(Player.cppと同じ値)
	const Vector3 player_hit_offset = Vector3(0.0f, 0.0f, -90.0f);
	constexpr float player_hit_radius = 50.0f;
	//被弾の原因とみなす、当たり判定どうしの離れ具合の上限
	constexpr float damage_cause_tolerance = 120.0f;

	//状態を記録する間隔
	constexpr int sample_interval = 30;
	//ボス登場とみなすZ座標の手前の余裕
	constexpr float boss_appear_margin = 100.0f;

	//書き出すファイル名
	constexpr const wchar_t* latest_js_name = L"latest.js";
	constexpr const wchar_t* status_file_name = L"status.txt";

	//ワイド文字列をUTF-8に変換する
	std::string ToUtf8(const std::wstring& text)
	{
		if (text.empty()) return std::string();
		int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
		std::string result(size, '\0');
		WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), result.data(), size, nullptr, nullptr);
		return result;
	}

	//JSONの文字列として書けるようにエスケープする
	std::string Quote(const std::string& text)
	{
		std::string result = "\"";
		for (char c : text)
		{
			if (c == '"' || c == '\\') result += '\\';
			result += c;
		}
		return result + "\"";
	}

	//小数を短く書く
	std::string Num(float value)
	{
		char buffer[32];
		std::snprintf(buffer, sizeof(buffer), "%.1f", value);
		return buffer;
	}

	//危険物の種類の名前
	const char* ThreatKindName(AutoPlayWorld::ThreatKind kind)
	{
		switch (kind)
		{
		case AutoPlayWorld::ThreatKind::EnemyBullet: return "EnemyBullet";
		case AutoPlayWorld::ThreatKind::BossBeam: return "BossBeam";
		case AutoPlayWorld::ThreatKind::BossBeamTip: return "BossBeamTip";
		case AutoPlayWorld::ThreatKind::Worm: return "Worm";
		case AutoPlayWorld::ThreatKind::Rock: return "Rock";
		}
		return "Unknown";
	}

	//今の日時を文字列にする
	std::string NowText(const char* format)
	{
		std::time_t now = std::time(nullptr);
		std::tm local = {};
		localtime_s(&local, &now);
		char buffer[64];
		std::strftime(buffer, sizeof(buffer), format, &local);
		return buffer;
	}
}

PlaytestRecorder::PlaytestRecorder()
{
}

PlaytestRecorder::~PlaytestRecorder()
{
}

void PlaytestRecorder::BeginSession(const SessionInfo& info, const std::wstring& outputDir)
{
	m_info = info;
	m_outputDir = outputDir;
	m_startedAt = NowText("%Y-%m-%d %H:%M:%S");
	std::string fileName = "session_" + NowText("%Y%m%d_%H%M%S") + ".json";
	m_sessionFileName = std::wstring(fileName.begin(), fileName.end());
	m_runs.clear();
	m_layout.clear();
	m_isLayoutRecorded = false;

	std::error_code error;
	std::filesystem::create_directories(m_outputDir, error);
}

void PlaytestRecorder::BeginRun(int runIndex)
{
	m_currentRun = Run();
	m_currentRun.index = runIndex;
	m_isRunActive = true;
	m_decidedResult = nullptr;
	m_isStageStarted = false;
	m_stageOffsetZ = 0.0f;
	m_lastStageZ = 0.0f;
	m_runFrame = 0;
	m_isPlayerSeen = false;
	m_isBossPhase = false;
	m_currentRun.bossAppearFrame = -1;
	m_lastDamageCause.clear();
}

void PlaytestRecorder::UpdateRun(const AutoPlayWorld& world)
{
	if (!m_isRunActive) return;

	const std::shared_ptr<Player>& pPlayer = world.GetPlayer();
	if (!pPlayer) return;

	//最初に見たときの値を基準にする
	if (!m_isPlayerSeen)
	{
		m_isPlayerSeen = true;
		m_prevHitCount = pPlayer->GetHitCount();
		m_prevHealth = pPlayer->GetHealth();
		m_prevCounterCount = pPlayer->GetCounterSuccessCount();
		m_prevDefeatedCount = pPlayer->GetDefeatedEnemyCount();
		m_prevChargeDefeatedCount = pPlayer->GetChargeShotDefeatedCount();
		m_wasRolling = pPlayer->IsRolling();
		m_wasInvincible = pPlayer->IsInvincible();
		m_wasBoosting = pPlayer->IsBoosting();
		m_wasBraking = pPlayer->IsBraking();
		m_wasSomersault = pPlayer->IsSomersault();
		m_wasDisabled = pPlayer->IsDisabled();
		m_wasBossDying = false;
	}

	m_runFrame++;

	//岩が出てきたら通常ステージの開始(そのときのプレイヤーのZが敵や岩のずらし量)
	if (!m_isStageStarted && world.HasRocks())
	{
		m_isStageStarted = true;
		m_stageOffsetZ = pPlayer->GetPos().z;
		m_currentRun.stageStartFrame = m_runFrame;

		//ステージ構成は最初の1回だけ記録する
		if (!m_isLayoutRecorded)
		{
			m_isLayoutRecorded = true;
			for (const AutoPlayWorld::Threat& threat : world.GetThreats())
			{
				if (threat.kind != AutoPlayWorld::ThreatKind::Rock) continue;
				m_layout.push_back({ "rock", threat.pos.x, threat.pos.y, threat.pos.z - m_stageOffsetZ, threat.radius });
			}
			for (const AutoPlayWorld::Enemy& enemy : world.GetAllEnemyPositions())
			{
				m_layout.push_back({ enemy.isWorm ? "worm" : "floating", enemy.pos.x, enemy.pos.y, enemy.pos.z - m_stageOffsetZ, 0.0f });
			}
		}
		AddEvent(world, "stage_start");
	}

	//結果が出た後は記録しない
	if (m_decidedResult) return;

	if (m_isStageStarted)
	{
		m_lastStageZ = pPlayer->GetPos().z - m_stageOffsetZ;
		if (m_lastStageZ > m_currentRun.reachedZ) m_currentRun.reachedZ = m_lastStageZ;
	}

	//被弾
	int hitCount = pPlayer->GetHitCount();
	int health = pPlayer->GetHealth();
	while (m_prevHitCount < hitCount)
	{
		m_lastDamageCause = GuessDamageCause(world);
		AddEvent(world, "damage", m_lastDamageCause, m_prevHealth - health);
		m_prevHitCount++;
	}
	m_prevHealth = health;

	//死亡
	if (pPlayer->IsDead())
	{
		AddEvent(world, "death", m_lastDamageCause);
		m_currentRun.deathCause = m_lastDamageCause;
		m_decidedResult = "death";
		return;
	}

	//敵弾のカウンター成功
	int counterCount = pPlayer->GetCounterSuccessCount();
	while (m_prevCounterCount < counterCount)
	{
		AddEvent(world, "counter", "EnemyBullet");
		m_prevCounterCount++;
	}
	//ボスのビームのカウンター成功(ローリング中に無敵時間が始まったら)
	bool isRolling = pPlayer->IsRolling();
	bool isInvincible = pPlayer->IsInvincible();
	if (isInvincible && !m_wasInvincible && (isRolling || m_wasRolling))
	{
		AddEvent(world, "counter", "BossBeam");
	}
	//ローリング(カウンターの入力)
	if (isRolling && !m_wasRolling) AddEvent(world, "roll");
	m_wasRolling = isRolling;
	m_wasInvincible = isInvincible;

	//敵を倒した
	int defeatedCount = pPlayer->GetDefeatedEnemyCount();
	int chargeDefeatedCount = pPlayer->GetChargeShotDefeatedCount();
	int chargeKills = chargeDefeatedCount - m_prevChargeDefeatedCount;
	for (int i = m_prevDefeatedCount; i < defeatedCount; i++)
	{
		bool isCharge = chargeKills > 0;
		if (isCharge) chargeKills--;
		AddEvent(world, "kill", isCharge ? "ChargeShot" : "Shot");
	}
	m_prevDefeatedCount = defeatedCount;
	m_prevChargeDefeatedCount = chargeDefeatedCount;

	//ゲージを使う行動
	bool isBoosting = pPlayer->IsBoosting();
	bool isBraking = pPlayer->IsBraking();
	bool isSomersault = pPlayer->IsSomersault();
	if (isBoosting && !m_wasBoosting) AddEvent(world, "boost");
	if (isBraking && !m_wasBraking) AddEvent(world, "brake");
	if (isSomersault && !m_wasSomersault) AddEvent(world, "somersault");
	m_wasBoosting = isBoosting;
	m_wasBraking = isBraking;
	m_wasSomersault = isSomersault;

	//ボス登場(ボスの位置まで来て操作できなくなったら)
	bool isDisabled = pPlayer->IsDisabled();
	if (m_isStageStarted && !m_isBossPhase && isDisabled && !m_wasDisabled &&
		m_lastStageZ > GameScene::boss_appear_z - boss_appear_margin)
	{
		AddEvent(world, "boss_appear");
		//ここから先の出来事はボス戦として印を付ける(ボス戦中もZは進み続けるため)
		m_isBossPhase = true;
		m_currentRun.bossAppearFrame = m_runFrame;
	}
	m_wasDisabled = isDisabled;

	//ボス撃破
	const std::shared_ptr<BossEnemy>& pBoss = world.GetBoss();
	bool isBossDying = pBoss && pBoss->IsDying();
	if (isBossDying && !m_wasBossDying)
	{
		AddEvent(world, "boss_defeated");
		m_decidedResult = "clear";
	}
	m_wasBossDying = isBossDying;

	//一定間隔で状態を残す(HPの推移のグラフ用)
	if (m_isStageStarted && m_runFrame % sample_interval == 0)
	{
		Sample sample;
		sample.frame = m_runFrame;
		sample.z = m_lastStageZ;
		sample.health = health;
		sample.gauge = pPlayer->GetSpecialGauge();
		m_currentRun.samples.push_back(sample);
	}
}

void PlaytestRecorder::EndRun(const char* result)
{
	if (!m_isRunActive) return;

	m_currentRun.result = result ? result : (m_decidedResult ? m_decidedResult : "aborted");
	m_currentRun.frames = m_runFrame;
	m_runs.push_back(m_currentRun);
	m_isRunActive = false;

	//途中で止められてもそこまでの記録が残るよう、1回ごとに書き出す
	WriteFiles();
}

int PlaytestRecorder::GetClearCount() const
{
	int count = 0;
	for (const Run& run : m_runs)
	{
		if (run.result == "clear") count++;
	}
	return count;
}

void PlaytestRecorder::WriteStatus(int runIndex, int runCount, bool isFinished) const
{
	std::ofstream file(std::filesystem::path(m_outputDir) / status_file_name, std::ios::binary | std::ios::trunc);
	if (!file) return;
	file << "run=" << runIndex << "\n";
	file << "runs=" << runCount << "\n";
	file << "done=" << m_runs.size() << "\n";
	file << "clears=" << GetClearCount() << "\n";
	file << "z=" << static_cast<int>(m_lastStageZ) << "\n";
	file << "hp=" << m_prevHealth << "\n";
	file << "finished=" << (isFinished ? 1 : 0) << "\n";
	file << "session=" << ToUtf8(m_sessionFileName) << "\n";
}

std::string PlaytestRecorder::GuessDamageCause(const AutoPlayWorld& world) const
{
	const std::shared_ptr<Player>& pPlayer = world.GetPlayer();
	Vector3 hitCenter = pPlayer->GetPos() + player_hit_offset;

	//当たり判定どうしが一番深く重なっている(近い)ものを原因とみなす
	const char* bestName = "Unknown";
	float bestGap = damage_cause_tolerance;
	for (const AutoPlayWorld::Threat& threat : world.GetThreats())
	{
		if (threat.kind == AutoPlayWorld::ThreatKind::BossBeamTip) continue;
		float gap = (threat.pos - hitCenter).Length() - threat.radius - player_hit_radius;
		if (gap < bestGap)
		{
			bestGap = gap;
			bestName = ThreatKindName(threat.kind);
		}
	}
	return bestName;
}

void PlaytestRecorder::AddEvent(const AutoPlayWorld& world, const char* type, const std::string& cause, int value)
{
	const std::shared_ptr<Player>& pPlayer = world.GetPlayer();
	Event event;
	event.type = type;
	event.frame = m_runFrame;
	event.z = m_isStageStarted ? pPlayer->GetPos().z - m_stageOffsetZ : 0.0f;
	event.x = pPlayer->GetPos().x;
	event.y = pPlayer->GetPos().y;
	event.cause = cause;
	event.value = value;
	event.health = pPlayer->GetHealth();
	event.isBossPhase = m_isBossPhase;
	m_currentRun.events.push_back(event);
}

void PlaytestRecorder::WriteFiles() const
{
	std::string json = BuildJson();
	std::filesystem::path dir(m_outputDir);

	//1回のテストごとのファイル(あとで比べるとき用)
	{
		std::ofstream file(dir / m_sessionFileName, std::ios::binary | std::ios::trunc);
		if (file) file << json;
	}
	//最新のテストをレポート画面がそのまま読み込めるようにする(file://ではJSONを直接読めないためJSにする)
	{
		std::ofstream file(dir / latest_js_name, std::ios::binary | std::ios::trunc);
		if (file) file << "window.NOVAWING_PLAYTEST = " << json << ";\n";
	}
}

std::string PlaytestRecorder::BuildJson() const
{
	std::ostringstream out;
	out << "{\n";
	out << "\"tool\":\"NovaWing AutoPlay\",\"version\":1,\n";
	out << "\"startedAt\":" << Quote(m_startedAt) << ",\n";
	out << "\"file\":" << Quote(ToUtf8(m_sessionFileName)) << ",\n";
	out << "\"settings\":{"
		<< "\"skill\":" << Quote(ToUtf8(m_info.skillName))
		<< ",\"speed\":" << Quote(ToUtf8(m_info.speedText))
		<< ",\"mode\":" << Quote(ToUtf8(m_info.modeText))
		<< ",\"runs\":" << m_info.runCount
		<< ",\"seed\":" << m_info.seed
		<< ",\"fps\":60},\n";
	out << "\"stage\":{\"bossAppearZ\":" << Num(GameScene::boss_appear_z) << ",\"layout\":[";
	for (size_t i = 0; i < m_layout.size(); i++)
	{
		const LayoutItem& item = m_layout[i];
		if (i > 0) out << ",";
		out << "{\"type\":" << Quote(item.type)
			<< ",\"x\":" << Num(item.x) << ",\"y\":" << Num(item.y) << ",\"z\":" << Num(item.z)
			<< ",\"r\":" << Num(item.radius) << "}";
	}
	out << "]},\n";
	out << "\"runs\":[\n";
	for (size_t i = 0; i < m_runs.size(); i++)
	{
		const Run& run = m_runs[i];
		if (i > 0) out << ",\n";
		out << "{\"index\":" << run.index
			<< ",\"result\":" << Quote(run.result)
			<< ",\"frames\":" << run.frames
			<< ",\"stageStartFrame\":" << run.stageStartFrame
			<< ",\"bossAppearFrame\":" << run.bossAppearFrame
			<< ",\"reachedZ\":" << Num(run.reachedZ)
			<< ",\"deathCause\":" << Quote(run.deathCause)
			<< ",\"events\":[";
		for (size_t e = 0; e < run.events.size(); e++)
		{
			const Event& event = run.events[e];
			if (e > 0) out << ",";
			out << "{\"t\":" << Quote(event.type)
				<< ",\"f\":" << event.frame
				<< ",\"z\":" << Num(event.z)
				<< ",\"x\":" << Num(event.x)
				<< ",\"y\":" << Num(event.y);
			if (!event.cause.empty()) out << ",\"cause\":" << Quote(event.cause);
			if (event.value != 0) out << ",\"v\":" << event.value;
			if (event.isBossPhase) out << ",\"boss\":1";
			out << ",\"hp\":" << event.health << "}";
		}
		out << "],\"samples\":[";
		for (size_t s = 0; s < run.samples.size(); s++)
		{
			const Sample& sample = run.samples[s];
			if (s > 0) out << ",";
			out << "[" << sample.frame << "," << Num(sample.z) << "," << sample.health << "," << Num(sample.gauge) << "]";
		}
		out << "]}";
	}
	out << "\n]\n}\n";
	return out.str();
}
#endif
