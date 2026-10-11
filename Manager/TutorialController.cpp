#include "TutorialController.h"

#include "../Game/GameObjects/Actors/Character/Player/Player.h"
#include "../Scene/SceneController.h"
#include "Scene/TutorialMessageScene.h"
#include "Manager/InputManager.h"
#include "Manager/UIManager.h"
#include "Game/UI/TutorialUI.h"

namespace
{
	//次のステップに移るまでの時間(成功を表示しておく間の時間)
	constexpr int to_next_step_frame = 60 * 2;
}

TutorialController::TutorialController(
	std::weak_ptr<Player> pPlayer,
	SceneController& controller,
	std::weak_ptr<UIManager> uiManager) :
	m_pPlayer(pPlayer),
	m_controller(controller)
{
	//仮
	StepData data;
	data.triggerZ = 2000.0f;
	data.message = L"移動しろ！";
	data.taskType = TaskType::Move;
	m_steps.push_back(data);
	data.triggerZ = 3000.0f;
	data.message = L"傾けろ！";
	data.taskType = TaskType::Tilt;
	m_steps.push_back(data);

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
		}
		break;
	case StepState::Trying:
		//タスクを表示させる(UI)
		m_tutorialUI->OnStart(m_steps[m_stepIndex].message);
		
		//仮でチュートリアルボタンを押したら成功とする
		if (InputManager::GetInstance().IsTriggered(InputEvent::Tutorial))
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