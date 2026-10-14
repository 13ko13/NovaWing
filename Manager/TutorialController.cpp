#include <cmath>

#include "TutorialController.h"
#include "../Game/GameObjects/Actors/Character/Player/Player.h"
#include "../Scene/SceneController.h"
#include "Scene/TutorialMessageScene.h"
#include "Manager/UIManager.h"
#include "Game/UI/TutorialUI.h"
#include "Utility/Vector2.h"

namespace
{
	//次のステップに移るまでの時間(成功を表示しておく間の時間)
	constexpr int to_next_step_frame = 60 * 2;
	//プレイヤーが移動タスクを完了したかの判定用の距離閾値
	constexpr float move_task_distance = 500.0f;
	//プレイヤーが機体を傾けるタスクを完了したかの判定用の角度閾値
	constexpr float tilt_task_angle = DX_PI_F / 4;
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
		{ 1300.0f, L"左スティックで移動しろ！", TaskType::Move, {} },
		{ 2800.0f, L"LBかRBを長押しして傾けろ！", TaskType::Tilt, {} },
		{ 4300.0f, L"LBかRBを2回押してバレルロールしろ！回転中は敵の弾を跳ね返せる！", TaskType::BarrelRoll,
			{
				{ Vector3(-300.0f, 0.0f, 2500.0f), EnemyFactory::EnemyType::FloatingEnemy },
				{ Vector3( 300.0f, 0.0f, 2500.0f), EnemyFactory::EnemyType::FloatingEnemy },
			} },
		{ 5800.0f, L"Xでブーストしろ！", TaskType::Boost, {} },
		{ 9200.0f, L"Aでブレーキしろ！", TaskType::Brake, {} },
		{ 10500.0f, L"Bで敵を撃ち落とせ！", TaskType::Shot,
			{
				{ Vector3(-400.0f, 0.0f, 2500.0f), EnemyFactory::EnemyType::FloatingEnemy },
				{ Vector3(   0.0f, 0.0f, 2500.0f), EnemyFactory::EnemyType::FloatingEnemy },
				{ Vector3( 400.0f, 0.0f, 2500.0f), EnemyFactory::EnemyType::FloatingEnemy },
			} },
		{ 12600.0f, L"Bを長押しして溜め、もう一度Bでチャージショットを撃って敵を倒せ！", TaskType::ChargeShot,
			{
				{ Vector3(-300.0f,    0.0f, 2500.0f), EnemyFactory::EnemyType::FloatingEnemy },
				{ Vector3(   0.0f,    0.0f, 2500.0f), EnemyFactory::EnemyType::FloatingEnemy },
				{ Vector3( 300.0f,    0.0f, 2500.0f), EnemyFactory::EnemyType::FloatingEnemy },
				{ Vector3(   0.0f,  300.0f, 2500.0f), EnemyFactory::EnemyType::FloatingEnemy },
				{ Vector3(   0.0f, -300.0f, 2500.0f), EnemyFactory::EnemyType::FloatingEnemy },
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
	case StepState::WaitTrigger:
		//プレイヤーの座標がステップデータのトリガーZを越えたら遷移する
		if (pPlayer->GetPos().z > m_steps[m_stepIndex].triggerZ)
		{
			//チュートリアルメッセージシーンをPushする
			m_controller.PushScene(std::make_shared<TutorialMessageScene>(m_controller, m_steps[m_stepIndex].message));
			//ステートの変更
			m_stepState = StepState::Trying;
			//プレイヤーの現在位置を覚えておく
			m_taskStartPos = pPlayer->GetPos();
			//通常弾で倒した敵の数を覚えておく
			m_normalShotDefeatedStartCount = pPlayer->GetNormalShotDefeatedCount();
			//チャージショットで倒した敵の数を覚えておく
			m_chargeShotDefeatedStartCount = pPlayer->GetChargeShotDefeatedCount();
			//カウンター成功回数を覚えておく
			m_counterSuccessCount = pPlayer->GetCounterSuccessCount();
		}
		break;
	case StepState::Trying:
		//タスクを表示させる(UI)
		//表示中なら UI 側で無視される
		m_tutorialUI->OnStart(m_steps[m_stepIndex].message);
		//タスクが終了していたら成功ステートに遷移
		if (IsTaskCleared(*pPlayer))
		{
			m_stepState = StepState::Success;
			m_tutorialUI->OnSuccess();
		}

		break;
	case StepState::Success:
		//2秒ぐらいで次のステップへ移動する
		m_successFrame++;
		if (m_successFrame > to_next_step_frame)
		{
			//次のステップに移動すると同時に、他の情報をリセット
			m_stepIndex++;
			m_successFrame = 0;
			m_stepState = StepState::WaitTrigger;
			m_tutorialUI->OnEnd();
		}
		break;
	}
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
		//プレイヤーがバレルロールしていればtrueを返す
		return player.IsRolling();

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
