#include <cmath>

#include "TutorialController.h"
#include "../Game/GameObjects/Actors/Character/Player/Player.h"
#include "../Scene/SceneController.h"
#include "Scene/TutorialMessageScene.h"
#include "Manager/UIManager.h"
#include "Game/UI/TutorialUI.h"
#include "Utility/Vector2.h"
#include "Game/GameObjects/Actors/Character/Enemy/EnemyFactory.h"

namespace
{
	//成功を表示しておく間の時間
	constexpr int success_show_frame = 60 * 2;
	//プレイヤーが移動タスクを完了したかの判定用の距離閾値
	constexpr float move_task_distance = 500.0f;
	//プレイヤーが機体を傾けるタスクを完了したかの判定用の角度閾値
	constexpr float tilt_task_angle = DX_PI_F / 4;

	//次のタスクに移るまでの時間
	constexpr int wait_next_step_frame = 60;
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
	//仮(後でTutorialData.csvから読み込む)
	//プレイヤーはZ=1200から毎フレーム8進む(約480/秒)。成功後2秒で次へ進むので、間隔は約1500以上空ける
	//ブースト・ブレーキはゲージ満タンが必要(回復は約3.3秒)なので、ブレーキはブーストから離しておく
	//敵はステップ開始時にプレイヤーの位置+offsetに出す。浮遊敵は距離4000未満で出現するので、前方2500ならすぐ出てくる
	//チャージの爆発半径は700なので、チャージの的は左右±300に収めて1発でまとめて倒せるようにする
	m_steps =
	{
		{ L"左スティックで移動しろ！", TaskType::Move,{} },
		{ L"LBかRBを長押しして傾けろ！", TaskType::Tilt,{} },
		{ L"LBかRBを2回押してバレルロールしろ！回転中は敵の弾を跳ね返せる！", TaskType::BarrelRoll,
			{
				{ Vector3(-300.0f, 0.0f, 2500.0f), EnemyFactory::EnemyType::FloatingEnemy },
				{ Vector3(300.0f, 0.0f, 2500.0f), EnemyFactory::EnemyType::FloatingEnemy },
			} },
		{ L"Xでブーストしろ！", TaskType::Boost,{} },
		{ L"Aでブレーキしろ！", TaskType::Brake,{} },
		{ L"Bで敵を撃ち落とせ！", TaskType::Shot,
			{
				{ Vector3(-400.0f, 0.0f, 2500.0f), EnemyFactory::EnemyType::FloatingEnemy },
				{ Vector3(0.0f, 0.0f, 2500.0f), EnemyFactory::EnemyType::FloatingEnemy },
				{ Vector3(400.0f, 0.0f, 2500.0f), EnemyFactory::EnemyType::FloatingEnemy },
			} },
		{ L"Bを長押しして溜め、もう一度Bでチャージショットを撃って敵を倒せ！", TaskType::ChargeShot,
			{
				{ Vector3(-300.0f,    0.0f, 2500.0f), EnemyFactory::EnemyType::FloatingEnemy },
				{ Vector3(0.0f,    0.0f, 2500.0f), EnemyFactory::EnemyType::FloatingEnemy },
				{ Vector3(300.0f,    0.0f, 2500.0f), EnemyFactory::EnemyType::FloatingEnemy },
				{ Vector3(0.0f,  300.0f, 2500.0f), EnemyFactory::EnemyType::FloatingEnemy },
				{ Vector3(0.0f, -300.0f, 2500.0f), EnemyFactory::EnemyType::FloatingEnemy },
			} },
	};

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
	if (m_stepIndex >= static_cast<int>(m_steps.size())) return;

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
		m_tutorialUI->OnStart(m_steps[m_stepIndex].message);
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
