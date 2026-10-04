#include "DxLib.h"
#include "LoadingScene.h"
#include "Manager/ResourceLoader.h"
#include "SceneController.h"

namespace
{
	//ロード画面を最低限表示し続けるフレーム
	constexpr int min_loading_frame = 240;

	//文字点滅の周期
	constexpr int blink_max_frame = 60;
	//半透明とするアルファ値
	constexpr int half_transparent_alpha = 128;
}

LoadingScene::LoadingScene(SceneController& controller, SceneID prevSceneID,
		std::shared_ptr<Scene> nextScene, float fadeFrame) :
	Scene(controller),
	m_prevSceneID(prevSceneID),
	m_nextScene(nextScene),
	m_fadeFrame(fadeFrame)
{
}

LoadingScene::~LoadingScene()
{
}

void LoadingScene::Init()
{
	m_elapsedFrame = 0;
	m_blinkFrame = 0;

	//非同期ロード開始
	ResourceLoader::GetInstance().BeginAsyncLoad();
	ResourceLoader::GetInstance().OnSceneChange(
		m_prevSceneID, m_nextScene->GetSceneID()
	);
}

void LoadingScene::Update()
{
	//経過フレームインクリメント
	m_elapsedFrame++;
	//点滅用フレーム更新
	m_blinkFrame++;

	//非同期ロード中か
	bool isAsyncLoading = ResourceLoader::GetInstance().IsAsyncLoading();
	//最低でもmin_loading_frameを越えているか
	bool isMinTimePassed = m_elapsedFrame >= min_loading_frame;

	//非同期ロード中が終わっていて、最低時間を越えているか
	if(!isAsyncLoading && isMinTimePassed)
	{
		//非同期ロードを終了してシーンを次のシーンに遷移
		ResourceLoader::GetInstance().EndAsyncLoad();
		m_controller.ChangeSceneDirect(m_nextScene, m_fadeFrame);
	}
}

void LoadingScene::Draw()
{
	//文字を点滅させる
	//文字が半透明か
	bool isHalfTransparent = (m_blinkFrame % blink_max_frame) >= blink_max_frame / 2;

	//アルファ値
	//半透明だったら128にする
	int alpha = 255;
	if (isHalfTransparent) alpha = half_transparent_alpha;

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	DrawString(50, 50, L"SYSTEM INITIALIZING", 0x00ffaa);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

SceneID LoadingScene::GetSceneID() const
{
	return SceneID::Loading;
}
