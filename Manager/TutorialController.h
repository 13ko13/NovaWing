#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Utility/Vector3.h"

class Player;
class SceneController;
class TutorialUI;
class UIManager;

/// チュートリアルを全て制御するクラス
class TutorialController
{
public:
	enum class TaskType {
		None,
		Move,
		Tilt,
		BarrelRoll,
		Boost,
		Brake,
		Shot,
		ChargeShot
	};

	TutorialController(
		std::weak_ptr<Player> pPlayer,
		SceneController& controller,
		std::weak_ptr<UIManager> uiManager
		);
	virtual ~TutorialController();

	//更新
	void Update();

private:
	//タスクが終っているか
	bool IsTaskCleared(const Player& player) const;

	//各ステップ内の状態
	enum class StepState
	{
		WaitTrigger,//トリガーZに来るのを待つ
		Trying,//挑戦中
		Success,//成功
	};
	//現在のステップ内の状態
	StepState m_stepState = StepState::WaitTrigger;

	//ステップ一つ分のデータ
	struct StepData
	{
		float triggerZ;
		std::wstring message;
		TaskType taskType;
	};

	std::weak_ptr<Player> m_pPlayer;
	SceneController& m_controller;
	int m_stepIndex = 0;

	//ステップごとの情報を配列で持つ
	std::vector<StepData> m_steps;

	//成功演出用のフレーム
	int m_successFrame = 0;

	//タスクとして表示しておくときのUI
	std::shared_ptr<TutorialUI> m_tutorialUI;

	//タスク開始時のプレイヤーの位置
	Vector3 m_taskStartPos;
	//タスク開始時に、通常弾で倒していた敵の数
	int m_normalShotDefeatedStartCount = 0;
	//タスク開始時に、チャージショットで倒していた敵の数
	int m_chargeShotDefeatedStartCount = 0;
};
