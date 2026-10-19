#include "AutoPlayBot.h"
#ifdef _DEBUG
#include <algorithm>
#include <cfloat>
#include <climits>
#include <cmath>
#include <vector>

#include <DxLib.h>

#include "AutoPlayWorld.h"
#include "Constants/Game.h"
#include "Game/GameObjects/Actors/Character/Player/Player.h"
#include "Game/GameObjects/Camera/CameraBase.h"

namespace
{
	//プレイヤーの当たり判定の中心のずれと半径(Player.cppと同じ値)
	const Vector3 player_hit_offset = Vector3(0.0f, 0.0f, -90.0f);
	constexpr float player_hit_radius = 50.0f;
	//カウンターの判定の半径(Player.cppと同じ値)
	constexpr float counter_radius = 100.0f;
	//プレイヤーの移動範囲を求めるときの距離(Player.cppのclamp_frustum_distzと同じ値)
	constexpr float clamp_frustum_dist_z = 900.0f;

	//ローリングボタンを1回目に押してから回転が始まるまでのフレーム
	constexpr int counter_start_delay = 2;
	//ローリングで1回転するフレーム(DefaultRotationStateと同じ値)
	constexpr int roll_frame = 20;
	//カウンターしてから次のカウンターを考えるまでの待ち
	constexpr int counter_cool_frame = 24;
	//カウンターのくじの記録をためておく上限
	constexpr size_t max_counter_decisions = 2000;

	//チャージ:押し続けるフレーム(長押し判定10F + チャージ完了20F + 余裕)
	constexpr int charge_hold_frame = 36;
	//チャージ:離しておくフレーム
	constexpr int charge_release_frame = 2;
	//チャージ:次のチャージまでの待ち
	constexpr int charge_cool_frame = 90;
	//チャージを使う、前方の敵の数
	constexpr int charge_enemy_count = 2;
	//チャージ中でも避けるのを優先する危険の近さ
	constexpr int charge_safe_frame = 10;

	//狙う敵の前後の範囲
	constexpr float target_min_dist_z = 150.0f;
	constexpr float target_max_dist_z = 2600.0f;
	//狙う敵を選ぶときの奥行きの重み
	constexpr float target_dist_z_weight = 0.3f;
	//狙いに向かうときのスティックの強さ(この距離で最大まで倒す)
	constexpr float aim_stick_full_dist = 120.0f;
	//弾が出る位置の前方へのずれ(NormalShootStateと同じ値)
	constexpr float fire_forward_offset = 200.0f;
	//撃つと判断する、弾の通り道と敵のずれの余裕
	constexpr float fire_tolerance = 30.0f;
	//狙いのずれを変える間隔
	constexpr int aim_offset_change_frame = 45;

	//何もないときに戻る位置と、そこへ向かうスティックの強さの基準距離
	const Vector2 home_pos = Vector2(0.0f, 300.0f);
	constexpr float home_stick_full_dist = 400.0f;

	//スティックを倒す量(XInputの最大値に対する割合)の上限
	constexpr float stick_max = 1.0f;

	//腕前の一覧
	//(起動引数を読むのはWinMainより前なので、グローバル変数の初期化順に頼らないよう関数内のstaticにする)
	const std::vector<AutoPlayBot::Skill>& GetSkillPresets()
	{
		static const std::vector<AutoPlayBot::Skill> presets =
		{
			//名前,反応の遅れ,予測,余裕,カウンター確率,狙いのずれ,チャージ,射撃間隔
			{ L"beginner", 14, 26, 10.0f, 0.2f, 140.0f, false, 14 },
			{ L"normal", 8, 40, 25.0f, 0.6f, 60.0f, true, 9 },
			{ L"expert", 3, 60, 40.0f, 0.95f, 15.0f, true, 6 },
		};
		return presets;
	}

	//プレイヤーの移動の速さ(MovingStateと同じ値)
	constexpr float player_move_speed = 19.0f;
	//避ける方向を探すときに試す方向の数
	constexpr int dodge_direction_count = 16;
	//すき間を調べる間隔(フレーム)
	constexpr int clearance_check_step = 2;
	//これより前後に離れているものは、すき間の計算に入れない
	constexpr float clearance_ignore_dist_z = 1600.0f;

	//避ける方向を探すときに見る危険物
	struct PlanThreat
	{
		Vector3 pos;
		Vector3 vel;
		float radius;
	};

	//スティックをstickの向きに倒し続けたとき、危険物とのすき間が一番狭くなる値を求める
	//(反応の遅れの間は今の速度のまま動くとする。移動範囲の端では止まる)
	float EvaluateClearance(const std::vector<PlanThreat>& threats, const Vector3& start, const Vector3& currentVel,
		const Vector2& stick, int delayFrame, int lookAheadFrame, const Vector2& moveHalf, float minY)
	{
		float minClearance = FLT_MAX;
		Vector3 pos = start;
		Vector3 moveVel(stick.x * player_move_speed, stick.y * player_move_speed, currentVel.z);
		for (int frame = 1; frame <= lookAheadFrame; frame++)
		{
			const Vector3& vel = (frame <= delayFrame) ? currentVel : moveVel;
			pos += vel;
			pos.x = std::clamp(pos.x, -moveHalf.x, moveHalf.x);
			pos.y = std::clamp(pos.y, minY, moveHalf.y);
			if (frame % clearance_check_step != 0) continue;

			Vector3 hitCenter = pos + player_hit_offset;
			for (const PlanThreat& threat : threats)
			{
				Vector3 threatPos = threat.pos + threat.vel * static_cast<float>(frame);
				float clearance = (threatPos - hitCenter).Length() - threat.radius - player_hit_radius;
				if (clearance < minClearance) minClearance = clearance;
			}
		}
		return minClearance;
	}

	float Clamp1(float value)
	{
		return std::clamp(value, -stick_max, stick_max);
	}
}

AutoPlayBot::AutoPlayBot()
{
}

AutoPlayBot::~AutoPlayBot()
{
}

bool AutoPlayBot::FindSkill(const std::wstring& name, Skill& outSkill)
{
	for (const Skill& skill : GetSkillPresets())
	{
		if (skill.name == name)
		{
			outSkill = skill;
			return true;
		}
	}
	return false;
}

void AutoPlayBot::Reset(const Skill& skill, unsigned int seed)
{
	m_skill = skill;
	m_random.seed(seed);

	//遅れの分だけ何も押していない入力を詰めておく
	m_delayQueue.clear();
	for (int i = 0; i < m_skill.reactionFrame; i++)
	{
		m_delayQueue.push_back(PadOutput());
	}

	m_counterStep = 0;
	m_counterCoolFrame = 0;
	m_counterDecisions.clear();
	m_chargeFrame = -1;
	m_chargeCoolFrame = 0;
	m_fireCoolFrame = 0;
	m_isShootPressedPrev = false;
	m_aimOffset = Vector2();
	m_aimOffsetFrame = 0;
	m_actionName = L"";
}

AutoPlayBot::PadOutput AutoPlayBot::Update(const AutoPlayWorld& world)
{
	//今フレームの判断を行列の後ろに入れて、遅れの分だけ前の判断を取り出す
	m_delayQueue.push_back(Decide(world));
	PadOutput output = m_delayQueue.front();
	m_delayQueue.pop_front();
	return output;
}

AutoPlayBot::PadOutput AutoPlayBot::Decide(const AutoPlayWorld& world)
{
	PadOutput output;

	const std::shared_ptr<Player>& pPlayer = world.GetPlayer();
	//操作できないとき(ボス登場演出など)は何もしない
	if (!pPlayer || pPlayer->IsDead() || pPlayer->IsDisabled())
	{
		m_counterStep = 0;
		m_chargeFrame = -1;
		m_isShootPressedPrev = false;
		m_actionName = L"待機";
		return output;
	}

	m_counterCoolFrame--;
	m_chargeCoolFrame--;
	m_fireCoolFrame--;

	//くじの記録が増えすぎたら捨てる(消えた弾の分がたまっていくため)
	if (m_counterDecisions.size() > max_counter_decisions)
	{
		m_counterDecisions.clear();
	}

	const Vector3 playerPos = pPlayer->GetPos();
	const Vector3 hitCenter = playerPos + player_hit_offset;
	const Vector3 playerVel = pPlayer->GetVel();

	//------------------------------------------------------------
	//危険の予測
	//------------------------------------------------------------
	//反応が遅い分、カウンターは早めに入力を始める
	const int counterEarliest = m_skill.reactionFrame + counter_start_delay;
	const int counterLatest = counterEarliest + roll_frame - 6;

	std::vector<PlanThreat> planThreats;//避ける方向を探すときに見るもの
	int nearestDangerFrame = INT_MAX;//一番近い危険までのフレーム
	bool isWantCounter = false;
	float counterSideX = 0.0f;

	for (const AutoPlayWorld::Threat& threat : world.GetThreats())
	{
		if (threat.isDead) continue;
		if (std::abs(threat.pos.z - playerPos.z) > clearance_ignore_dist_z) continue;

		//相手から見たプレイヤーとの位置関係を、等速で動くとして予測する
		Vector3 relPos = threat.pos - hitCenter;
		Vector3 relVel = threat.vel - playerVel;
		float relSpeedSq = Vector3::Dot(relVel, relVel);
		float closestFrame = 0.0f;
		if (relSpeedSq > 0.0001f)
		{
			closestFrame = -Vector3::Dot(relPos, relVel) / relSpeedSq;
			closestFrame = std::clamp(closestFrame, 0.0f, static_cast<float>(m_skill.lookAheadFrame));
		}
		Vector3 closest = relPos + relVel * closestFrame;
		float closestDist = closest.Length();
		int frame = static_cast<int>(closestFrame);

		//カウンターできるもの(敵弾・ビームの先端)
		bool isCounterable = threat.kind == AutoPlayWorld::ThreatKind::EnemyBullet ||
			threat.kind == AutoPlayWorld::ThreatKind::BossBeamTip;
		bool isHandledByCounter = false;
		if (isCounterable && closestDist < threat.radius + counter_radius)
		{
			//同じ弾には一度だけくじを引く
			auto it = m_counterDecisions.find(threat.key);
			if (it == m_counterDecisions.end())
			{
				it = m_counterDecisions.emplace(threat.key, Random01() < m_skill.counterChance).first;
			}
			bool isDecided = it->second;

			if (isDecided && frame >= counterEarliest && frame <= counterLatest)
			{
				isWantCounter = true;
				counterSideX = -closest.x;
			}
			//カウンターするつもりの弾、ローリング中に届く弾は避けない
			isHandledByCounter = isDecided || pPlayer->IsRolling();
		}

		//ビームの先端は当たってもダメージがないので避けない
		if (threat.kind == AutoPlayWorld::ThreatKind::BossBeamTip) continue;
		if (isHandledByCounter && frame <= counterLatest) continue;

		planThreats.push_back({ threat.pos, threat.vel, threat.radius });

		float dangerDist = threat.radius + player_hit_radius + m_skill.dodgeMargin;
		if (closestDist < dangerDist) nearestDangerFrame = (std::min)(nearestDangerFrame, frame);
	}

	//------------------------------------------------------------
	//移動
	//------------------------------------------------------------
	//移動できる範囲(Player::ClampPositionと同じ求め方)
	Vector2 moveHalf(1000.0f, 1000.0f);
	if (std::shared_ptr<CameraBase> pCamera = pPlayer->GetCamera().lock())
	{
		moveHalf = pCamera->GetFrustumHalfSize(clamp_frustum_dist_z);
	}
	float minY = (std::max)(-moveHalf.y, Game::sea_player_margin);

	//狙う敵を選ぶ
	const AutoPlayWorld::Enemy* pTarget = nullptr;
	float bestScore = FLT_MAX;
	int aheadEnemyCount = 0;
	for (const AutoPlayWorld::Enemy& enemy : world.GetEnemies())
	{
		float distZ = enemy.pos.z - playerPos.z;
		if (distZ < target_min_dist_z || distZ > target_max_dist_z) continue;
		aheadEnemyCount++;
		float distXY = Vector2(enemy.pos.x - playerPos.x, enemy.pos.y - playerPos.y).Length();
		float score = distXY + distZ * target_dist_z_weight;
		if (score < bestScore)
		{
			bestScore = score;
			pTarget = &enemy;
		}
	}

	//狙いのずれをときどき変える
	m_aimOffsetFrame--;
	if (m_aimOffsetFrame <= 0)
	{
		m_aimOffset = Vector2(
			(Random01() * 2.0f - 1.0f) * m_skill.aimError,
			(Random01() * 2.0f - 1.0f) * m_skill.aimError);
		m_aimOffsetFrame = aim_offset_change_frame;
	}

	//本当は行きたい方向(敵を狙う、なければ真ん中に戻る)
	Vector2 desired;
	if (pTarget)
	{
		Vector2 diff(
			pTarget->pos.x + m_aimOffset.x - playerPos.x,
			pTarget->pos.y + m_aimOffset.y - playerPos.y);
		desired = diff * (1.0f / aim_stick_full_dist);
		m_actionName = L"狙う";
	}
	else
	{
		Vector2 diff(home_pos.x - playerPos.x, home_pos.y - playerPos.y);
		desired = diff * (1.0f / home_stick_full_dist);
		m_actionName = L"巡航";
	}
	desired = Vector2(Clamp1(desired.x), Clamp1(desired.y));

	//行きたい方向に動いて危険物とのすき間が足りなければ、すき間が一番空く方向を探して避ける
	auto clearanceOf = [&](const Vector2& stickDir)
		{
			return EvaluateClearance(planThreats, playerPos, playerVel, stickDir,
				m_skill.reactionFrame, m_skill.lookAheadFrame, moveHalf, minY);
		};
	Vector2 stick = desired;
	if (!planThreats.empty() && clearanceOf(desired) < m_skill.dodgeMargin)
	{
		float desiredLength = desired.Length();
		Vector2 desiredDir = (desiredLength > 0.0001f) ? desired * (1.0f / desiredLength) : Vector2();
		float bestClearance = -FLT_MAX;
		float bestFollow = -FLT_MAX;
		//止まる + 全方向に倒す、を試す
		for (int i = -1; i < dodge_direction_count; i++)
		{
			Vector2 candidate;
			if (i >= 0)
			{
				float angle = DX_TWO_PI_F * static_cast<float>(i) / dodge_direction_count;
				candidate = Vector2(std::cos(angle), std::sin(angle));
			}
			float clearance = clearanceOf(candidate);
			//すき間が同じくらいなら、行きたい方向に近い方を選ぶ
			float follow = candidate.x * desiredDir.x + candidate.y * desiredDir.y;
			bool isEnoughBoth = clearance >= m_skill.dodgeMargin && bestClearance >= m_skill.dodgeMargin;
			bool isBetter = isEnoughBoth ? (follow > bestFollow) : (clearance > bestClearance);
			if (isBetter)
			{
				bestClearance = clearance;
				bestFollow = follow;
				stick = candidate;
			}
		}
		m_actionName = L"回避";
	}

	output.stickX = Clamp1(stick.x);
	output.stickY = Clamp1(stick.y);

	//------------------------------------------------------------
	//カウンター
	//------------------------------------------------------------
	if (isWantCounter && m_counterStep == 0 && m_counterCoolFrame <= 0 && !pPlayer->IsRolling())
	{
		m_counterStep = 1;
		//弾が来る側と反対に回る(見た目だけの違い)
		m_counterButton = (counterSideX >= 0.0f) ? XINPUT_BUTTON_RIGHT_SHOULDER : XINPUT_BUTTON_LEFT_SHOULDER;
	}
	UpdateCounterInput(output);

	//------------------------------------------------------------
	//射撃
	//------------------------------------------------------------
	bool isSafeForCharge = nearestDangerFrame > charge_safe_frame;
	if (m_chargeFrame < 0 && m_skill.isUseCharge && m_chargeCoolFrame <= 0 &&
		aheadEnemyCount >= charge_enemy_count && isSafeForCharge)
	{
		m_chargeFrame = 0;
	}

	if (m_chargeFrame >= 0)
	{
		UpdateChargeInput(output);
		m_actionName = L"チャージ";
	}
	else if (pTarget && m_fireCoolFrame <= 0 && !m_isShootPressedPrev)
	{
		//弾の通り道(機体の向き)が敵の近くを通るときだけ撃つ
		//(ボスの周りの盾に当てるとボスが回復するので、むやみに撃たない)
		Vector3 forward = pPlayer->GetVisualForward();
		Vector3 firePos = playerPos + Vector3(0.0f, 0.0f, fire_forward_offset);
		Vector3 toEnemy = pTarget->pos - firePos;
		float along = Vector3::Dot(toEnemy, forward);
		float gap = (toEnemy - forward * along).Length();
		if (along > 0.0f && gap < pTarget->radius + fire_tolerance + m_skill.aimError * 0.5f)
		{
			output.buttons[XINPUT_BUTTON_B] = true;
			m_fireCoolFrame = m_skill.fireInterval;
		}
	}
	m_isShootPressedPrev = output.buttons[XINPUT_BUTTON_B];

	if (m_counterStep > 0) m_actionName = L"カウンター";

	return output;
}

void AutoPlayBot::UpdateCounterInput(PadOutput& output)
{
	if (m_counterStep == 0) return;

	//押す→離す→押す→離す
	output.buttons[m_counterButton] = (m_counterStep == 1 || m_counterStep == 3);
	m_counterStep++;
	if (m_counterStep > 4)
	{
		m_counterStep = 0;
		m_counterCoolFrame = counter_cool_frame;
	}
}

void AutoPlayBot::UpdateChargeInput(PadOutput& output)
{
	//長押し→離す→押す(発射)→離す
	const int fireFrame = charge_hold_frame + charge_release_frame;
	if (m_chargeFrame < charge_hold_frame)
	{
		output.buttons[XINPUT_BUTTON_B] = true;
	}
	else if (m_chargeFrame == fireFrame)
	{
		output.buttons[XINPUT_BUTTON_B] = true;
	}
	else
	{
		output.buttons[XINPUT_BUTTON_B] = false;
	}

	m_chargeFrame++;
	if (m_chargeFrame > fireFrame + 1)
	{
		m_chargeFrame = -1;
		m_chargeCoolFrame = charge_cool_frame;
	}
}

float AutoPlayBot::Random01()
{
	return std::uniform_real_distribution<float>(0.0f, 1.0f)(m_random);
}
#endif
