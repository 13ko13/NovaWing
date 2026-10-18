#include <cmath>
#include <unordered_map>
#include <cassert>

#include "TutorialController.h"
#include "../Game/GameObjects/Actors/Character/Player/Player.h"
#include "../Scene/SceneController.h"
#include "Scene/TutorialMessageScene.h"
#include "Manager/UIManager.h"
#include "Game/UI/TutorialUI.h"
#include "Utility/Vector2.h"
#include "Game/GameObjects/Actors/Character/Enemy/EnemyFactory.h"
#include "Manager/CSVDataLoader.h"
#include "CSVData/TutorialStepData.h"
#include "CSVData/TutorialEnemyData.h"

namespace
{
	//成功を表示しておく間の時間
	constexpr int success_show_frame = 60 * 2;
	//プレイヤーが移動タスクを完了したかの判定用の距離閾値
	constexpr float move_task_distance = 100.0f;
	//プレイヤーが機体を傾けるタスクを完了したかの判定用の角度閾値
	constexpr float tilt_task_angle = DX_PI_F / 4;

	//次のタスクに移るまでの時間
	constexpr int wait_next_step_frame = 60;

	TutorialController::TaskType ToTaskType(const std::wstring& name)
	{
		static const std::unordered_map<
			std::wstring, TutorialController::TaskType> taskTable =
		{
			{ L"Move",TutorialController::TaskType::Move },
			{ L"Tilt",TutorialController::TaskType::Tilt },
			{ L"BarrelRoll",TutorialController::TaskType::BarrelRoll },
			{ L"Boost",TutorialController::TaskType::Boost },
			{ L"Brake",TutorialController::TaskType::Brake },
			{ L"Somersault",TutorialController::TaskType::Somersault },
			{ L"Shot",TutorialController::TaskType::Shot },
			{ L"ChargeShot",TutorialController::TaskType::ChargeShot },
		};

		//taskTableから受け取った名前でTaskTypeを探す
		auto it = taskTable.find(name);
		//見つかったらTaskType、見つからなければassert & Noneを返す
		if (it != taskTable.end())
		{
			return it->second;
		}
		else
		{
			assert(false && "登録されていないタスクです。");
			return TutorialController::TaskType::None;
		}
	}

	//文字列で探してEnemyTypeを返す
	EnemyFactory::EnemyType ToEnemyType(const std::wstring& name)
	{
		const static std::unordered_map<
			std::wstring, EnemyFactory::EnemyType> enemyTable =
		{
			{ L"FloatingEnemy",EnemyFactory::EnemyType::FloatingEnemy },
			{ L"WormEnemy",EnemyFactory::EnemyType::WormEnemy }
		};

		//enemyTableを使ってnameでEnemyTypeを探す
		auto it = enemyTable.find(name);
		//見つかればEnemyType、見つからなければassert & Noneを返す
		if (it != enemyTable.end())
		{
			return it->second;
		}
		else
		{
			assert(false && "登録されていない敵の種類です。");
			return EnemyFactory::EnemyType::None;
		}
	}
}

TutorialController::TutorialController(
	std::weak_ptr<Player> pPlayer,
	SceneController& controller,
	std::weak_ptr<UIManager> uiManager,
		std::shared_ptr<EnemyFactory> pEnemyFactory) :
	m_pPlayer(pPlayer),
	m_controller(controller),
	m_pEnemyFactory(pEnemyFactory)
{
	//Tutorial.csvの1行ずつをStepDataに詰め替えてm_stepsに入れる
	auto stepData = CSVDataLoader::GetInstance().LoadCSV(L"Stage1/Tutorial");
	for (auto& row : stepData)
	{
		//その行の文字列をステップ用のデータクラスに変換
		TutorialStepData csvStep = TutorialStepData(row);

		//StepDataに1項目ずつ入れる
		StepData step;
		step.message = csvStep.GetMessageText();
		step.taskText = csvStep.GetTaskText();
		step.taskType = ToTaskType(csvStep.GetTaskType());//文字列からtaskTypeに変換
		//spawnDataはTutorialEnemy.csvを読み込むときに入れる

		//stepIDは0から順番に振ること(Enemyの添え字でそのまま使用するから)
		//ずれていればassertで知らせる
		assert(
			csvStep.GetStepID() == static_cast<int>(m_steps.size()) &&
			 "ステップIDがそろっていません"
		);

		m_steps.push_back(step);
	}

	//TutorialEnemy.csvの1行(敵1体)ずつ、対応するステップのspawnDataに入れる
	auto enemyData = CSVDataLoader::GetInstance().LoadCSV(L"Stage1/TutorialEnemy");
	for (auto& row : enemyData)
	{
		TutorialEnemyData csvEnemy = TutorialEnemyData(row);

		//敵の種類を変換。Noneなら(assertは中で済んでいるので)飛ばす
		EnemyFactory::EnemyType type = ToEnemyType(csvEnemy.GetEnemyType());
		if (type == EnemyFactory::EnemyType::None) continue;
		int stepID = csvEnemy.GetStepID();
		if (stepID < 0 || static_cast<int>(m_steps.size()) <= stepID)
		{
			assert(false && "TutorialEnemy.csvのstepIDに対応するステップがありません");
			continue;
		}
		//種類とオフセットを保持
		SpawnData spawnData;
		spawnData.offset = csvEnemy.GetOffset();
		spawnData.type = type;

		m_steps[stepID].spawnData.push_back(spawnData);
	}

	//tutorialUIの実体を確保
	m_tutorialUI = std::make_shared<TutorialUI>();

	//UIManagerに登録
	uiManager.lock()->Register(m_tutorialUI);
}

TutorialController::~TutorialController()
{
}

void TutorialController::Update()
{
	//全部終わっているなら何もしない
	if (IsFinished()) return;

	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();
	if (!pPlayer) return;

	switch (m_stepState)
	{
	case StepState::WaitNextStep:
	{
		m_appearNextStepFrame++;
		if (m_appearNextStepFrame < wait_next_step_frame) break;

		//チュートリアルメッセージシーンをPushする
		m_controller.PushScene(std::make_shared<TutorialMessageScene>(m_controller, m_steps[m_stepIndex].message));
		//ステートの変更
		m_stepState = StepState::Trying;
		//カウンターリセット
		m_appearNextStepFrame = 0;
		//プレイヤーの現在位置を覚えておく
		m_taskStartPos = pPlayer->GetPos();
		//通常弾で倒した敵の数を覚えておく
		m_normalShotDefeatedStartCount = pPlayer->GetNormalShotDefeatedCount();
		//チャージショットで倒した敵の数を覚えておく
		m_chargeShotDefeatedStartCount = pPlayer->GetChargeShotDefeatedCount();
		//カウンター成功回数を覚えておく
		m_counterSuccessStartCount = pPlayer->GetCounterSuccessCount();

		//ブースト、ブレーキ、宙返りならゲージを満タンにする
		if (m_steps[m_stepIndex].taskType == TaskType::Boost ||
			m_steps[m_stepIndex].taskType == TaskType::Brake ||
			m_steps[m_stepIndex].taskType == TaskType::Somersault)
		{
			//減っている分増やす
			float deltaGauge = pPlayer->GetMaxSpecialGauge() - pPlayer->GetSpecialGauge();
			pPlayer->ChangeGauge(deltaGauge);
		}

		SpawnTaskEnemies(*pPlayer);
	}
	break;
	case StepState::Trying:
	{
		//プレイヤーがタスクをクリアしているか
		bool isCleared = IsTaskCleared(*pPlayer);

		//もしまだ成功していなくて
		//このステップのspawnDataが空ではなく、
		//タスク用の敵が全員いない場合であれば敵を出す
		if (!isCleared &&
			!m_steps[m_stepIndex].spawnData.empty() &&
			IsDeadAllTaskEnemies())
		{
			//敵を出現
			SpawnTaskEnemies(*pPlayer);
		}

		//タスクを表示させる(UI)
		//表示中なら UI 側で無視される
		m_tutorialUI->OnStart(m_steps[m_stepIndex].taskText);
		//タスクが終了していたら成功ステートに遷移
		if (isCleared)
		{
			m_stepState = StepState::Success;
			m_tutorialUI->OnSuccess();
		}
	}

	break;
	case StepState::Success:
		//2秒ぐらいで次のステップへ移動する
		m_successFrame++;
		if (m_successFrame > success_show_frame)
		{
			//次のステップに移動すると同時に、他の情報をリセット
			m_stepIndex++;
			m_successFrame = 0;
			m_stepState = StepState::WaitNextStep;
			m_tutorialUI->OnEnd();
		}
		break;
	}
}

bool TutorialController::IsFinished() const
{
	//ステップが終了していれば
	return m_stepIndex >= static_cast<int>(m_steps.size());
}

void TutorialController::SpawnTaskEnemies(const Player& player)
{
	//敵を生成する前に前回の敵を消しておく
	m_pTaskEnemies.clear();
	std::shared_ptr<EnemyFactory> pEnemyFactory = m_pEnemyFactory.lock();
	//SpawnData情報をもとに敵を召喚する
	if (pEnemyFactory)
	{
		for (const auto& data : m_steps[m_stepIndex].spawnData)
		{
			//生成した敵を持っておく
			m_pTaskEnemies.push_back(pEnemyFactory->Create(
				player.GetPos() + data.offset, data.type));
		}
	}
}

bool TutorialController::IsDeadAllTaskEnemies() const
{
	//すべての敵をループして参照先が存在するものが一体でもいれば
	//まだすべての敵が死んだわけではないのでfalseを返す
	for (const auto& enemy : m_pTaskEnemies)
	{
		if (!enemy.expired()) return false;
	}

	return true;
}

bool TutorialController::IsTaskCleared(const Player& player) const
{
	switch (m_steps[m_stepIndex].taskType)
	{
	case TaskType::Move:
	{
		//プレイヤーの位置
		Vector3 playerPos = player.GetPos();
		//タスク開始時の位置から一定距離以上動いていればタスククリアとする
		Vector3 moveDelta = playerPos - m_taskStartPos;
		//XとYだけを見た移動距離を出す
		float distanceXY = Vector2(moveDelta.x, moveDelta.y).Length();
		//移動距離がタスク完了判定の距離を越えていればクリア
		return distanceXY > move_task_distance;
	}
	case TaskType::Tilt:
	{
		//プレイヤーのZ軸回転が一定の角度を越えていればクリアとする
		float rotZ = player.GetRotationZ();

		return std::abs(rotZ) > tilt_task_angle;
	}
	case TaskType::BarrelRoll:
		//タスク開始からカウンターの回数が増えていればクリア(カウンターは必中なので敵を殺せる)
		return player.GetCounterSuccessCount() > m_counterSuccessStartCount;

	case TaskType::Boost:
		//プレイヤーがブーストしていればtrueを返す
		return player.IsBoosting();

	case TaskType::Brake:
		//プレイヤーがブレーキしていればtrueを返す
		return player.IsBraking();

	case TaskType::Somersault:
		return player.IsSomersault();

	case TaskType::Shot:
		//タスク開始から通常弾で敵を倒していたらタスククリアとする
		return player.GetNormalShotDefeatedCount() > m_normalShotDefeatedStartCount;

	case TaskType::ChargeShot:
		//タスク開始からチャージショットで敵を倒していたらタスククリアとする
		return player.GetChargeShotDefeatedCount() > m_chargeShotDefeatedStartCount;

	default:
		return false;
	}
}
