#define NOMINMAX

#include <DxLib.h>
#include <cassert>
#include <string>
#include <cmath>
#include <algorithm>
#include <EffekseerForDXLib.h>

#include "../Game/GameObjects/Actors/Actor.h"
#include "../Game/GameObjects/Actors/Charactor/Charactor.h"
#include "../Game/GameObjects/Actors/Charactor/Player/Player.h"
#include "GameScene.h"
#include "../Manager/InputManager.h"
#include "SceneController.h"
#include "../Main/Application.h"
#include "../Manager/BulletManager.h"
#include "../Game/GameObjects/Camera/GameCamera.h"
#include "../Manager/GameObjectManager.h"
#include "../Manager/ResourceLoader.h"
#include "Charactor/Enemy/FloatingEnemy/FloatingEnemy.h"
#include "Charactor/Enemy/WormEnemy/WormEnemy.h"
#include "Manager/CollisionManager.h"
#include "Scene/GameoverScene.h"
#include "Manager/TargetManager.h"
#include "Manager/UIManager.h"
#include "Game/UI/ReticleUI.h"
#include "Manager/WaterManager.h"
#include "Game/BackGround/SkyBox.h"
#include "Game/GameObjects/Actors/Rock/Rock.h"
#include "Manager/WaterRevealManager.h"
#include "Rock/RockDataSetter.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/FloatingEnemy/FloatingEnemyDataSetter.h"
#include "Stage/Stage.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/WormEnemy/WormEnemyDataSetter.h"
#include "Scene/ClearScene.h"
#include "Game/UI/PlayerHPGaugeUI.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/BossEnemy/BossEnemy.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/BossEnemy/BossEnemyDataSetter.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/EnemyFactory.h"
#include "Game/UI/BossHPGaugeUI.h"
#include "Manager/LightingManager.h"

namespace
{
	//グリッドのサイズ
	const Vector3 grid_size = { 1400.0f, 0.0f, 1400.0f };

	//1秒あたりのフレーム数
	constexpr int frame_per_second = 60;
	//プレイヤーの位置がどのあたりに行ったらクリアにするか(仮)
	constexpr float clear_pos_z = 30000.0f;

	//ボスを登場タイミングz座標(プレイヤー位置)
	constexpr float boss_appear_z = 20000.0f;

	//地震のような強く長い揺れ
	//ボス登場時のカメラを揺らす力
	constexpr float boss_appear_shake_power = 7.0f;
	//ボス登場時のカメラを揺らす時間
	constexpr int boss_appear_shake_frame = 60 * 2;

	//衝撃のような短く少し強い揺れ
	//ボスが着地した時の揺らす力
	constexpr float boss_landing_shake_power = 55.0f;
	//ボスが着地した時の揺らすフレーム
	constexpr float boss_landing_shake_frame = 20;

	//ボスを落下させるときの速度
	constexpr float boss_fall_speed = 4.0f;

	//ボスへのズームの速度
	constexpr float boss_zoom_speed = 0.04f;
	//ボスへのターゲットオフセットY
	constexpr float boss_target_offset_y = 1500.0f;

	//ボス登場のズーム時に保たせる最低限の距離
	const float boss_appear_zoom_limit = 1000.0f;
	//ボス死亡のズーム時に保たせる最低限の距離
	const float boss_death_zoom_limit = 2500.0f;

	//ライトの方向
	const Vector3 light_direction = Vector3(1.0f, -1.0f, 0.6f);
}

GameScene::GameScene(SceneController& controller) :
	Scene(controller),
	m_frame(0)
{
	//BulletManagerは先に生成しておかないとプレイヤーが生成できないので
	//コンストラクタで生成しておく
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

	//リソースローダーのインスタンス
	ResourceLoader& resourceL = ResourceLoader::GetInstance();

	//プレイヤーを生成
	m_pPlayer = std::make_shared<Player>(
		m_pBulletManager, ResourceLoader::ModelID::Player,
	std::weak_ptr<CameraBase>());//カメラがまだ生成されていないので空のweak_ptrを渡す
	//プレイヤーの初期化処理
	m_pPlayer->Init();

	//カメラへのポインタを確保
	m_pCamera = std::make_shared<GameCamera>(m_pPlayer);
	//カメラの初期化処理
	m_pCamera->Init();
	//カメラを生成したのでプレイヤーにカメラをセットする
	m_pPlayer->SetCamera(m_pCamera);

	//ボスエネミーの初期化
	m_pBoss = BossEnemyDataSetter::CreateEnemy(
		m_pPlayer,
		m_pCamera,
		m_pBulletManager);
	m_pBoss->Init();

	//衝突判定マネージャーの初期化
	m_pCollisionManager = std::make_shared<CollisionManager>(
		m_pPlayer,
		m_pBulletManager,
		m_pCamera,
		m_pBoss);

	//ターゲットマネージャーの初期化
	m_pTargetManager = std::make_shared<TargetManager>(m_pPlayer);
	//プレイヤーにターゲットマネージャーをセットする
	m_pPlayer->SetTargetManager(m_pTargetManager);

	//敵生産工場の初期化
	m_pEnemyFactory = std::make_shared<EnemyFactory>(
		m_pPlayer,
		m_pBulletManager,
		m_pCamera,
		m_pTargetManager,
		m_pCollisionManager
	);
	//ボスに工場をセットする
	m_pBoss->SetEnemyFactory(m_pEnemyFactory);

	//浮遊エネミーの初期化
	//データの数分のエネミーを作成
	m_pFloatingEnemies = FloatingEnemyDataSetter::CreateEnemy(
		m_pPlayer,
		m_pCamera,
		m_pBulletManager
	);
	//それぞれの初期化
	for (std::shared_ptr<FloatingEnemy> pEnemy : m_pFloatingEnemies)
	{
		pEnemy->Init();
		//衝突判定マネージャーに敵を登録する
		m_pCollisionManager->RegisterFloatingEnemy(pEnemy);
		//ターゲットマネージャーにエネミーを登録する
		m_pTargetManager->RegisterFloatingEnemy(pEnemy);
	}

	//ワームエネミーの初期化
	m_pWormEnemies = WormEnemyDataSetter::CreateEnemy(
		m_pPlayer,
		m_pCamera,
		m_pBulletManager
	);
	for (std::shared_ptr<WormEnemy> pEnemy : m_pWormEnemies)
	{
		pEnemy->Init();
		//衝突判定マネージャーに敵を登録する
		m_pCollisionManager->RegisterWormEnemy(pEnemy);
		//ターゲットマネージャーにエネミーを登録する
		m_pTargetManager->RegisterWormEnemy(pEnemy);
	}

	//UIManagerの初期化
	m_pUIManager = std::make_shared<UIManager>();
	m_pUIManager->Register(std::make_shared<ReticleUI>(m_pTargetManager, m_pPlayer));
	m_pUIManager->Register(std::make_shared<PlayerHPGaugeUI>(m_pPlayer));
	m_pUIManager->Register(std::make_shared<BossHPGaugeUI>(m_pBoss));

	//水マネージャーの初期化
	m_pWaterManager = std::make_shared<WaterManager>(m_pCamera);
	m_pWaterManager->Init();

	//スカイボックスの初期化
	m_pSkyBox = std::make_shared<SkyBox>(
		resourceL.GetGraphic(ResourceLoader::GraphicID::SkyBoxFront),
		resourceL.GetGraphic(ResourceLoader::GraphicID::SkyBoxBack),
		resourceL.GetGraphic(ResourceLoader::GraphicID::SkyBoxLeft),
		resourceL.GetGraphic(ResourceLoader::GraphicID::SkyBoxRight),
		resourceL.GetGraphic(ResourceLoader::GraphicID::SkyBoxUp),
		resourceL.GetGraphic(ResourceLoader::GraphicID::SkyBoxBottom)
	);

	//岩の生成&初期化
	m_pRocks = RockDataSetter::CreateRock(m_pCamera);
	//それぞれの岩の初期化
	for (std::shared_ptr<Rock> pRock : m_pRocks)
	{
		pRock->Init();
		//当たり判定のマネージャーに登録する
		m_pCollisionManager->RegisterRock(pRock);
	}

	//ステージの初期化
	m_pStage = std::make_shared<Stage>(ResourceLoader::ModelID::Stage,
	m_pCamera);
	m_pStage->Init();

	// ライトの方向ベクトルをセットする
	LightingManager::GetInstance().SetLightDirection(light_direction);
}

void GameScene::Update()
{
	//フレームカウンターの更新
	m_frame++;

	//全GameObjectのUpdateを呼ぶ
	GameObjectManager::GetInstance().UpdateAll();

	//Effekseerのエフェクト更新
	Effekseer_Sync3DSetting();
	UpdateEffekseer3D();

	//衝突判定マネージャーの更新
	m_pCollisionManager->Update();

	//バレットマネージャーの更新
	m_pBulletManager->Update();

	//タッゲットマネージャーの更新
	m_pTargetManager->Update();

	//水マネージャーの更新
	m_pWaterManager->Update();

	//UIマネージャーの更新
	m_pUIManager->Update();

	//プレイヤーが特定のz座標まで到達したら
	//カメラを揺らしてボスを登場させる
	if (m_pPlayer->GetPos().m_z > boss_appear_z)
	{
		//ボスが出現していない場合のみ行う
		if (!m_isApearBoss)
		{
			//プレイヤーが何も行わないようにする
			m_pPlayer->ChangeAllStateToDisabled();

			switch (m_bossApearState)
			{
			case BossApearState::None:
				//スタート時ステートに遷移
				m_bossApearState = BossApearState::Start;
				break;
			case BossApearState::Start:
				//カメラを揺らす(地震のように)
				m_pCamera->OnShake(boss_appear_shake_power, boss_appear_shake_frame);
				//出現ステートに遷移
				m_bossApearState = BossApearState::Apear;
				break;

			case BossApearState::Apear:
			{
				//揺れが止まっていれば出現させる
				if (!m_pCamera->IsShake())
				{
					//ボスを登場させる
					//重力を加える
					Vector3 currentVec = m_pBoss->GetVel();
					currentVec.m_y -= boss_fall_speed;

					m_pBoss->SetVel(currentVec);

					//ボスの最初の着地が完了していたら
					if (m_pBoss->IsFirstLanding())
					{
						m_bossApearState = BossApearState::Landing;
					}
				}
			}

			break;

			case BossApearState::Landing:
				//カメラを揺らす(衝撃)
				m_pCamera->OnShake(boss_landing_shake_power, boss_landing_shake_frame);
				//ステートをカメラズームに遷移
				m_bossApearState = BossApearState::CameraZoom;
				break;

			case BossApearState::CameraZoom:
				//カメラが揺れていないのを確認してから
				//カメラをズームさせる
				if (!m_pCamera->IsShake())
				{
					m_pCamera->OnZoomUp(
						boss_zoom_speed, 
						m_pBoss,
						boss_appear_zoom_limit,
						boss_target_offset_y);
					//ボス出現フラグを立てる
					m_isApearBoss = true;
					//ボスの行動を許可する
					m_pBoss->SetIsBossAppear(true);
					//プレイヤーも普通のステートに戻す
					m_pPlayer->ChangeAllStateToNormal();
				}
				break;
			}
		}
	}

	//プレイヤーが死んだらゲームオーバーシーンに遷移する
	if (m_pPlayer->IsDead())
	{
		m_controller.ChangeScene(
			std::make_shared<GameoverScene>(
				m_controller), frame_per_second);
	}

	//ボスを倒したらクリアにする
	if (m_pBoss->IsDead())
	{
		//リザルトに渡す情報を組み立てる
		ClearScene::ClearResultData resultData;
		resultData.clearTime = m_frame;
		resultData.defeatedEnemyCount = m_pPlayer->GetDefeatedEnemyCount();
		resultData.hitCount = m_pPlayer->GetHitCount();

		m_controller.ChangeScene(
			std::make_shared<ClearScene>(
				m_controller, resultData),
			frame_per_second);
	}

	//ボスが死亡待機状態になったら
	if (m_pBoss->IsDying())
	{
		//プレイヤーの進行を止める
		m_pPlayer->ChangeAllStateToDisabled();

		//カメラをズームする
		m_pCamera->OnZoomUp(
			boss_zoom_speed,
			m_pBoss,
			boss_death_zoom_limit,
			boss_target_offset_y
		);
	}

#ifdef _DEBUG
	//Startボタンでリスタート
	if (InputManager::GetInstance().IsTriggered(InputEvent::restart))
	{
		m_controller.ChangeScene(std::make_shared<GameScene>(m_controller), 0.0f);
	}
#endif
}

void GameScene::Draw()
{
	//WaterRevealManagerのインスタンスを取得
	WaterRevealManager& revealManager = WaterRevealManager::GetInstance();
	//キャプチャ開始
	revealManager.BeginCapture();
	//視野角とNearFarを再設定
	m_pCamera->SetUpCamera();
	//キャプチャのほうに全オブジェクトの描画を行う
	//これはキャプチャの方に描画しただけなので、のちにまたすべてを描画する必要がある
	GameObjectManager::GetInstance().DrawAll();

	//キャプチャを終了
	revealManager.EndCapture();
	//視野角とNearFarを再設定
	m_pCamera->SetUpCamera();

	//スカイボックスの描画
	m_pSkyBox->Draw(m_pCamera->GetPos());

	//グリッドの描画
	DrawGrid();

#ifdef _DEBUG
	DrawString(0, 0, L"GameScene", 0xffffff);
	DrawFormatString(0, 16, 0xffffff, L"FRAME:%d", m_frame);
#endif //DEBUG

	//全GameObjectのDrawを呼ぶ
	GameObjectManager::GetInstance().DrawAll();

	//水マネージャーの描画
	m_pWaterManager->Draw();

	//全てのUIを描画する
	m_pUIManager->Draw();

	//レティクルよりプレイヤーが優先的に描画されてほしいので
	//プレイヤーをもう一度描画する
	m_pPlayer->Draw();

	//Effekseerのエフェクト描画
	DrawEffekseer3D();
}

void GameScene::DrawGrid()
{
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