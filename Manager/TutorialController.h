#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Utility/Vector3.h"
#include "Game/GameObjects/Actors/Character/Enemy/EnemyFactory.h"

class Player;
class SceneController;
class TutorialUI;
class UIManager;
class EnemyBase;

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
		ChargeShot,
		Somersault,
	};

	TutorialController(
		std::weak_ptr<Player> pPlayer,
		SceneController& controller,
		std::weak_ptr<UIManager> uiManager,
		std::shared_ptr<EnemyFactory> pEnemyFactory,
		const std::wstring& csvFolderName
		);
	virtual ~TutorialController();

	//更新
	void Update();

	//チュートリアルが終わったか
	bool IsFinished() const;

	//チュートリアルを途中で打ち切って終了状態にする
	void Skip();

private:
	
	//タスクが終っているか
	bool IsTaskCleared(const Player& player) const;
	//タスク用の敵をスポーンさせる
	void SpawnTaskEnemies(const Player& player);
	//タスク用の敵がすべていなくなったか
	bool IsDeadAllTaskEnemies() const;

private:

	//各ステップ内の状態
	enum class StepState
	{
		WaitNextStep,//次のステップを待つ
		Trying,//挑戦中
		Success,//成功
	};
	//現在のステップ内の状態
	StepState m_stepState = StepState::WaitNextStep;

	//ステップ開始時に出す敵1体分のデータ
	struct SpawnData
	{
		Vector3 offset;//プレイヤーの位置からのずれ
		EnemyFactory::EnemyType type;//敵の種類
	};

	//ステップ一つ分のデータ
	struct StepData
	{
		std::wstring message;
		std::wstring taskText;
		TaskType taskType;
		std::vector<SpawnData> spawnData;//敵の数ぶんのデータ
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
	//タスク開始時のカウンター成功回数
	int m_counterSuccessStartCount = 0;

	//エネミー工場
	std::weak_ptr<EnemyFactory> m_pEnemyFactory;

	//タスクが終ってない状態で敵が消えたら巻き戻したいので
	//そのタスクに必要な敵のポインタを保持しておく
	std::vector<std::weak_ptr<EnemyBase>> m_pTaskEnemies;

	//タスククリア後に次のタスクを表示するまでのフレームカウンター
	int m_appearNextStepFrame = 0;
};
