#pragma once
#include <memory>

#include "Scene.h"

class LoadingScene :public Scene
{
public:
	LoadingScene(SceneController& controller, SceneID prevSceneID,
		std::shared_ptr<Scene> nextScene, float fadeFrame);
	~LoadingScene();

	void Init() override;
	void Update() override;
	void Draw() override;
	SceneID GetSceneID() const override;//自身のシーンIDを返す

private:
	//ロード前のシーンID(ResourceLoader::OnSceneChangeに渡すため)
	SceneID m_prevSceneID;
	//ロード完了後に切り替える本来の遷移先シーン
	std::shared_ptr<Scene> m_nextScene;
	//本来の遷移先シーンに切り替える際のフェードフレーム数
	float m_fadeFrame;

	//このシーンに入ってから経過したフレーム数
	int m_elapsedFrame = 0;

	//「ロード中...」のような文字の点滅用フレーム
	int m_blinkFrame = 0;
};
