#define NOMINMAX

#include <DxLib.h>
#include <cassert>
#include <string>
#include <cmath>
#include <algorithm>

#include "../Game/GameObjects/Actors/Actor.h"
#include "../Game/GameObjects/Actors/Charactor/Charactor.h"
#include "../Game/GameObjects/Actors/Charactor/Player/Player.h"
#include "GameScene.h"
#include "../Manager/InputManager.h"
#include "SceneController.h"
#include "../Main/Application.h"
#include "../Manager/BulletManager.h"
#include "../Game/GameObjects/Camera/CameraBase.h"
#include "../Manager/GameObjectManager.h"
#include "../Manager/ResourceLoader.h"
#include "Charactor/Enemy/FloatingEnemy/FloatingEnemy.h"
#include "Charactor/Enemy/WormEnemy/WormEnemy.h"
#include "Manager/CollisionManager.h"
#include "Scene/GameoverScene.h"
#include "Manager/TargetManager.h"
#include "Manager/UIManager.h"
#include "Game/UI/ReticleUI.h"

namespace
{
	//グリッドのサイズ
	const Vector3 grid_size = { 1400.0f, 0.0f, 1400.0f };

	//1秒あたりのフレーム数
	constexpr int frame_per_second = 60;
}

GameScene::GameScene(SceneController& controller) :
	Scene(controller),
	m_frame(0)
{
	//BulletManagerを生成
	m_pBulletManager = std::make_shared<BulletManager>();
}

GameScene::~GameScene()
{

}

void GameScene::Init()
{
	//ゲームオブジェクトマネージャーの初期化
	GameObjectManager::GetInstance().ClearAll();

	//カリングの設定
	SetUseBackCulling(true);
	
	//プレイヤーを生成
	m_pPlayer = std::make_shared<Player>(
		m_pBulletManager,ResourceLoader::ModelID::Player);
	//プレイヤーの初期化処理
	m_pPlayer->Init();

	//カメラの実体を確保
	m_pCamera = std::make_shared<CameraBase>(m_pPlayer);
	//カメラの初期化処理
	m_pCamera->Init();

	//プレイヤーにカメラを渡す
	m_pPlayer->SetCamera(m_pCamera);

	//衝突判定マネージャーの初期化
	m_pCollisionManager = std::make_shared<CollisionManager>(m_pPlayer, m_pBulletManager);

	//エネミーの初期化
	m_pFloatingEnemy = std::make_shared<FloatingEnemy>(m_pPlayer,
		ResourceLoader::ModelID::FloatingEnemy,
		m_pBulletManager);
	m_pFloatingEnemy->Init();
	//衝突判定マネージャーに敵を登録する
	m_pCollisionManager->RegisterFloatingEnemy(m_pFloatingEnemy);
	
	//ワームエネミーの初期化
	m_pWormEnemy = std::make_shared<WormEnemy>(m_pPlayer,
		ResourceLoader::ModelID::WormHead,
		m_pBulletManager, 5);
	m_pWormEnemy->Init();
	//衝突判定マネージャーにワームエネミーを登録する
	m_pCollisionManager->RegisterWormEnemy(m_pWormEnemy);

	//ターゲットマネージャーの初期化
	m_pTargetManager = std::make_shared<TargetManager>(m_pPlayer);
	//プレイヤーにターゲットマネージャーをセットする
	m_pPlayer->SetTargetManager(m_pTargetManager);
	//ターゲットマネージャーにエネミーを登録する
	m_pTargetManager->RegisterFloatingEnemy(m_pFloatingEnemy);
	m_pTargetManager->RegisterWormEnemy(m_pWormEnemy);

	//UIManagerの初期化
	m_pUIManager = std::make_shared<UIManager>();
	m_pUIManager->Register(std::make_shared<ReticleUI>(m_pTargetManager,m_pPlayer));
}

void GameScene::Update()
{
	//フレームカウンターの更新
	m_frame++;

	//全GameObjectのUpdateを呼ぶ
	GameObjectManager::GetInstance().UpdateAll();

	//衝突判定マネージャーの更新
	m_pCollisionManager->Update();

	//タッゲットマネージャーの更新
	m_pTargetManager->Update();

	//プレイヤーが死んだらゲームオーバーシーンに遷移する
	if (m_pPlayer->IsDead())
	{
		m_controller.ChangeScene(
			std::make_shared<GameoverScene>(
				m_controller), frame_per_second);
	}
}

void GameScene::Draw()
{
	//グリッドの描画
	DrawGrid();

#ifdef _DEBUG
	DrawString(0, 0, L"GameScene", 0xffffff);
	DrawFormatString(0, 16, 0xffffff, L"FRAME:%d", m_frame);
#endif //DEBUG

	//全GameObjectのDrawを呼ぶ
	GameObjectManager::GetInstance().DrawAll();

	//全てのUIを描画する
	m_pUIManager->Draw();
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
	for (int z = static_cast<int>(-grid_size.m_z);
		z <= static_cast<int>(grid_size.m_z); z += 100)
	{
		startPos = VGet(-grid_size.m_x, 0.0f, static_cast<float>(z));
		endPos = VGet(grid_size.m_x, 0.0f, static_cast<float>(z));
		DrawLine3D(startPos, endPos, 0xff0000);
	}
	for (int x = static_cast<int>(-grid_size.m_x); x <= static_cast<int>(grid_size.m_x); x += 100)
	{
		startPos = VGet(static_cast<float>(x), 0.0f, -grid_size.m_z);
		endPos = VGet(static_cast<float>(x), 0.0f, grid_size.m_z);
		DrawLine3D(startPos, endPos, 0x0000ff);
	}
#endif
}