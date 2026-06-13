#define NOMINMAX

#include <DxLib.h>
#include <cassert>
#include <string>
#include <cmath>
#include <algorithm>

#include "../Actor/Charactor.h"
#include "../Actor/Player/Player.h"
#include "GameScene.h"
#include "../Manager/InputManager.h"
#include "SceneController.h"
#include "../Main/Application.h"

namespace
{
	//カメラのnearとfar
	constexpr float camera_near = 200.0f;
	constexpr float camera_far = 1800.0f;

	//ステージのサイズ
	const Vector3 stage_size = { 1400.0f, 0.0f, 1400.0f };

	//地面の場所
	const Vector3 ground_pos = { 0.0f, -50.0f, 0.0f };

	//フェードにかけるフレーム数
	constexpr float fade_frame = 60.0f;

	//1秒あたりのフレーム数
	constexpr int frame_per_second = 60;
}

GameScene::GameScene(SceneController& controller) :
	Scene(controller),
	m_frameCount(0)
{
}

GameScene::~GameScene()
{

}

void GameScene::Init()
{
	//カリングの設定
	SetUseBackCulling(true);

	//TODO:ResourceLoaderから必要なリソースを取得して初期化する
	//auto& resourceLoader = ResourceLoader::GetInstance();

	//プレイヤーの実体を確保
	m_pPlayer = std::make_shared<Player>();
	m_pPlayer->Init();
	//カメラの実体を確保
	//m_pCamera = std::make_shared<Camera>(camera_target_pos);

	//全てのActorを格納する
	PushAllActor();
}

void GameScene::Update(InputManager& input)
{
	//フレームカウンターの更新
	m_frameCount++;

	//カメラの更新
	//m_pCamera->Update(m_pPlayer->GetTargetPos(), input);
}

void GameScene::Draw()
{
	//グリッドの描画
	DrawGrid();

#ifdef _DEBUG
	DrawString(0, 0, L"GameScene", 0xffffff);
	DrawFormatString(0, 16, 0xffffff, L"FRAME:%d", m_frameCount);
#endif //DEBUG

	//文字の描画(仮)
	//ウィンドウサイズを取得する
	auto& windowSize = Application::GetInstance().GetWindowSize();
}

void GameScene::DrawGrid()
{
	//カメラのdraw(基本デバッグ用)
	//m_pCamera->Draw();

#ifdef _DEBUG
	//直線の始点と終点
	VECTOR startPos;
	VECTOR endPos;

	//ステージのサイズに合わせてグリッドを描画する
	for (int z = static_cast<int>(-stage_size.m_z);
		z <= static_cast<int>(stage_size.m_z); z += 100)
	{
		startPos = VGet(-stage_size.m_x, 0.0f, static_cast<float>(z));
		endPos = VGet(-stage_size.m_x, 0.0f, static_cast<float>(z));
		DrawLine3D(startPos, endPos, 0xff0000);
	}
	for (int x = static_cast<int>(-stage_size.m_x); x <= static_cast<int>(stage_size.m_x); x += 100)
	{
		startPos = VGet(static_cast<float>(x), 0.0f, -stage_size.m_z);
		endPos = VGet(static_cast<float>(x), 0.0f, -stage_size.m_z);
		DrawLine3D(startPos, endPos, 0x0000ff);
	}
#endif
}

void GameScene::PushAllActor()
{
	//キャラクターを継承しているオブジェクトを格納
	m_pActors.push_back(m_pPlayer);
}
