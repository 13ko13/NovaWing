#pragma once
#ifdef _DEBUG
#include <array>
#include <deque>
#include <map>
#include <random>
#include <string>

#include "Utility/Vector2.h"

class AutoPlayWorld;

/// <summary>
/// AIテストプレイ用:ゲームの状態を見て、パッドの入力を決めるルールベースのAI
/// (避ける・狙って撃つ・チャージショット・カウンター)
/// </summary>
class AutoPlayBot
{
public:
	//AIの腕前
	struct Skill
	{
		std::wstring name;//表示名
		int reactionFrame = 0;//判断してから入力が反映されるまでの遅れ
		int lookAheadFrame = 0;//何フレーム先まで危険を予測するか
		float dodgeMargin = 0.0f;//当たり判定にどれだけ余裕をもって避けるか
		float counterChance = 0.0f;//カウンターできる場面で実際にカウンターする確率
		float aimError = 0.0f;//狙いのずれ(大きいほど下手)
		bool isUseCharge = false;//チャージショットを使うか
		int fireInterval = 0;//通常弾を撃つ間隔
	};

	//パッドの入力(ボタンの添字はXINPUT_BUTTON_*)
	struct PadOutput
	{
		std::array<bool, 16> buttons = {};
		float stickX = 0.0f;//右が正(-1～1)
		float stickY = 0.0f;//上が正(-1～1)
	};

	AutoPlayBot();
	virtual ~AutoPlayBot();

	//腕前の名前(beginner/normal/expert)から設定を取得する。知らない名前ならfalse
	static bool FindSkill(const std::wstring& name, Skill& outSkill);

	//1回のプレイの最初に呼ぶ
	void Reset(const Skill& skill, unsigned int seed);

	//ゲームシーン中に毎フレーム呼び、入力を決める
	PadOutput Update(const AutoPlayWorld& world);

	//今何をしているか(画面表示用)
	const wchar_t* GetActionName() const { return m_actionName; }

private:
	//今フレームの判断(遅れを入れる前)
	PadOutput Decide(const AutoPlayWorld& world);

	//カウンター入力を進める(ボタン2回押し)
	void UpdateCounterInput(PadOutput& output);
	//チャージショット入力を進める(長押し→離す→押す)
	void UpdateChargeInput(PadOutput& output);

	//0～1の乱数
	float Random01();

private:
	Skill m_skill;
	std::mt19937 m_random;

	//反応の遅れを作るための入力の待ち行列
	std::deque<PadOutput> m_delayQueue;

	//カウンターの入力段階(0:していない 1～4:2回押しの途中)
	int m_counterStep = 0;
	//カウンターに使うボタン
	int m_counterButton = 0;
	//次のカウンターまでの待ち
	int m_counterCoolFrame = 0;
	//カウンターするかどうかを一度決めた弾(同じ弾で何度もくじを引かない)
	std::map<const void*, bool> m_counterDecisions;

	//チャージショットの入力の経過フレーム(-1:していない)
	int m_chargeFrame = -1;
	//次のチャージショットまでの待ち
	int m_chargeCoolFrame = 0;

	//次の通常弾までの待ち
	int m_fireCoolFrame = 0;
	//前フレームに射撃ボタンを押したか(押しっぱなしだとチャージになるため)
	bool m_isShootPressedPrev = false;

	//狙いのずれ
	Vector2 m_aimOffset;
	//狙いのずれを変えるまでのフレーム
	int m_aimOffsetFrame = 0;

	//今何をしているか
	const wchar_t* m_actionName = L"";
};
#endif
