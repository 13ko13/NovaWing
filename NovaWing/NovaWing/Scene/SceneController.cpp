#include "SceneController.h"
#include "Scene.h"
#include "../Manager/InputManager.h"

void SceneController::ResetScene(std::shared_ptr<Scene> scene)
{
	//シーンを1つだけにしたいため、一旦シーンをすべてクリアします
	m_scenes.clear();
	m_scenes.push_back(scene);
}

void SceneController::ChangeScene(std::shared_ptr<Scene> scene, float fadeFrame)
{
	//シーンが積まれていないときはResetSceneする
	if(m_scenes.empty())
	{
		ResetScene(scene);
		Init();
		m_fade.StartFadeIn(fadeFrame);
		return;
	}

	//m_nextSceneに次のシーンを保存します
	m_nextScene = scene;

	//フェードマネージャーにフェードアウトを開始させます
	m_fade.StartFadeOut(fadeFrame);
}

void SceneController::PushScene(std::shared_ptr<Scene> scene)
{
	//新しいシーンを末尾に積みます(下から積むイメージ)
	m_scenes.push_back(scene);
}

void SceneController::PopScene()
{
	//末尾のシーンを削除する
	m_scenes.pop_back();
}

void SceneController::Init()
{
	//末尾の要素に対してInitする
	m_scenes.back()->Init();
}

void SceneController::Update()
{
	//フェードマネージャーの更新
	m_fade.Update();

	//フェードアウトが終了しているか
	if(!m_fade.IsFading() && m_nextScene != nullptr)
	{
		//フェードインを開始する
		m_fade.StartFadeIn(m_fade.GetFadeFrame());

		//次のシーンに切り替える
		ResetScene(m_nextScene);
		//次のシーンの初期化
		Init();

		m_nextScene = nullptr;
	}
	else
	{
		//末尾の要素に対してUpdateする
		m_scenes.back()->Update();
	}
}

void SceneController::Draw()
{
	//Updateだけ止めてすべてDraw
	//ポーズしたときにプレイ背景は残る
	for (auto& scene : m_scenes)
	{
		scene->Draw();
	}
	//フェードマネージャーの描画
	m_fade.Draw();
}