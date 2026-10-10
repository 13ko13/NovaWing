#define NOMINMAX

#include <DxLib.h>
#include <cassert>
#include <string>
#include <cmath>
#include <algorithm>

#include "../Game/GameObjects/Actors/Actor.h"
#include "../Game/GameObjects/Actors/Character/Character.h"
#include "../Game/GameObjects/Actors/Character/Player/Player.h"
#include "GameScene.h"
#include "SceneID.h"
#include "../Manager/InputManager.h"
#include "../Manager/DebugManager.h"
#include "SceneController.h"
#include "../Main/Application.h"
#include "../Manager/BulletManager.h"
#include "../Game/GameObjects/Camera/GameCamera.h"
#include "../Manager/GameObjectManager.h"
#include "../Manager/ResourceLoader.h"
#include "Character/Enemy/FloatingEnemy/FloatingEnemy.h"
#include "Character/Enemy/WormEnemy/WormEnemy.h"
#include "Game/GameObjects/Actors/Character/Enemy/EnemyBase.h"
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
#include "Game/GameObjects/Actors/Character/Enemy/FloatingEnemy/FloatingEnemyDataSetter.h"
#include "Stage/Stage.h"
#include "Game/GameObjects/Actors/Character/Enemy/WormEnemy/WormEnemyDataSetter.h"
#include "Scene/ClearScene.h"
#include "Game/UI/PlayerHPGaugeUI.h"
#include "Game/GameObjects/Actors/Character/Enemy/BossEnemy/BossEnemy.h"
#include "Game/GameObjects/Actors/Character/Enemy/BossEnemy/BossEnemyDataSetter.h"
#include "Game/GameObjects/Actors/Character/Enemy/EnemyFactory.h"
#include "Game/UI/BossHPGaugeUI.h"
#include "Manager/LightingManager.h"
#include "Constants/Game.h"
#include "Manager/SoundManager.h"
#include "Manager/EffectManager.h"
#include "Game/UI/SpecialGaugeUI.h"
#include "Game/UI/WarningUI.h"
#include "PauseScene.h"
#include "Manager/TutorialController.h"

namespace
{
	//グリッドのサイズ
	const Vector3 grid_size = { 1400.0f, 0.0f, 1400.0f };

	//1秒あたりのフレーム数
	constexpr int frame_per_second = 60;

	//ボスを登場タイミングz座標(プレイヤー位置)
	constexpr float boss_appear_z = 27000.0f;

	//地震のような強く長い揺れ
	//ボス登場時のカメラを揺らす力
	constexpr float boss_appear_shake_power = 7.0f;
	//ボス登場時のカメラを揺らす時間(この間WARNINGを出す)
	constexpr int boss_appear_shake_frame = 60 * 3;

	//ムービー終了後にボスを立たせる高さ(海面)
	constexpr float boss_stand_y = 0.0f;

	//ボスへのズームの速度
	constexpr float boss_zoom_speed = 0.04f;
	//ボスへのターゲットオフセットY
	constexpr float boss_target_offset_y = 1500.0f;

	//ボス登場ムービーの最後のカメラ位置(ボスの正面からの距離と高さ)
	//ムービー明けはこの位置から始めて、そこからプレイヤーの位置へ引いて戻る
	constexpr float boss_appear_camera_dist = 2700.0f;
	constexpr float boss_appear_camera_height = 1020.0f;
	//ムービーの最後の位置へ1フレームで移すためのズーム速度
	constexpr float boss_appear_zoom_speed = 1.0f;
	//ボス死亡のズーム時に保たせる最低限の距離
	const float boss_death_zoom_limit = 2500.0f;

	//地震音のフェードアウトにかける時間
	constexpr float boss_quake_fade_out_time = 30.0f;

	//ボス死亡待機状態になった時のBGMのフェードアウトにかける時間
	constexpr float boss_death_bgm_fade_out_time = 30.0f;
}

GameScene::GameScene(SceneController& controller) :
	Scene(controller),
	m_frame(0)
{
	//BulletManagerは先に生成しておかないとプレイヤーが生成できないので
	//コンストラクタで生成しておく
	//BulletManagerにEffectManagerを渡すので、EffectManagerはBulletManagerより先に生成する
	m_pEffectManager = std::make_shared<EffectManager>();
	m_pBulletManager = std::make_shared<BulletManager>(m_pEffectManager);

	//SoundManagerも同様にプレイヤーより先に生成しておく
	m_pSoundManager = std::make_shared<SoundManager>();
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

	//サウンドマネージャーの初期化
	m_pSoundManager->Init();

	//ゲームBGMを鳴らす
	m_pSoundManager->Play(SoundManager::SoundType::GameBGM, true);

	//プレイヤーを生成
	m_pPlayer = std::make_shared<Player>(
		m_pBulletManager, ResourceLoader::ModelID::Player,
		std::weak_ptr<CameraBase>(),//カメラがまだ生成されていないので空のweak_ptrを渡す
		m_pSoundManager,
		m_pEffectManager);

	//Initでターゲットマネージャーを必要とするので先に生成しておく
	//ターゲットマネージャーの初期化
	m_pTargetManager = std::make_shared<TargetManager>(m_pPlayer);

	//プレイヤーにターゲットマネージャーをセットする
	m_pPlayer->SetTargetManager(m_pTargetManager);

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
		m_pBulletManager,
		m_pSoundManager,
		m_pEffectManager);
	m_pBoss->Init();

	//衝突判定マネージャーの初期化
	m_pCollisionManager = std::make_shared<CollisionManager>(
		m_pPlayer,
		m_pBulletManager,
		m_pCamera,
		m_pBoss);

	//敵生産工場の初期化
	m_pEnemyFactory = std::make_shared<EnemyFactory>(
		m_pPlayer,
		m_pBulletManager,
		m_pCamera,
		m_pTargetManager,
		m_pCollisionManager,
		m_pSoundManager,
		m_pEffectManager
	);
	//ボスに工場をセットする
	m_pBoss->SetEnemyFactory(m_pEnemyFactory);

	//浮遊エネミーの初期化
	//データの数分のエネミーを作成
	std::vector<std::shared_ptr<EnemyBase>> floatingEnemies = FloatingEnemyDataSetter::CreateEnemy(
		m_pPlayer,
		m_pCamera,
		m_pBulletManager,
		m_pSoundManager,
		m_pEffectManager
	);
	//それぞれの初期化
	for (std::shared_ptr<EnemyBase> pEnemy : floatingEnemies)
	{
		pEnemy->Init();
		//衝突判定マネージャーに敵を登録する
		m_pCollisionManager->Register(pEnemy);
		//ターゲットマネージャーにエネミーを登録する
		m_pTargetManager->Register(pEnemy);
		//生存を保持するための配列に格納
		m_pEnemies.push_back(pEnemy);
	}

	//ワームエネミーの初期化
	std::vector<std::shared_ptr<EnemyBase>> wormEnemies = WormEnemyDataSetter::CreateEnemy(
		m_pPlayer,
		m_pCamera,
		m_pBulletManager,
		m_pSoundManager,
		m_pEffectManager
	);
	for (std::shared_ptr<EnemyBase> pEnemy : wormEnemies)
	{
		pEnemy->Init();
		//衝突判定マネージャーに敵を登録する
		m_pCollisionManager->Register(pEnemy);
		//ターゲットマネージャーにエネミーを登録する
		m_pTargetManager->Register(pEnemy);
		//生存を保持するための配列に格納
		m_pEnemies.push_back(pEnemy);
	}

	//UIManagerの初期化
	m_pUIManager = std::make_shared<UIManager>();
	m_pUIManager->Register(std::make_shared<ReticleUI>(m_pTargetManager, m_pPlayer));
	m_pUIManager->Register(std::make_shared<PlayerHPGaugeUI>(m_pPlayer));
	m_pUIManager->Register(std::make_shared<BossHPGaugeUI>(m_pBoss));
	m_pUIManager->Register(std::make_shared<SpecialGaugeUI>(m_pPlayer));
	//ボス登場前のWARNINGは、GameSceneから表示を開始するのでポインタを持っておく
	m_pWarningUI = std::make_shared<WarningUI>();
	m_pUIManager->Register(m_pWarningUI);

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
	LightingManager::GetInstance().SetLightDirection(Game::light_direction); 

	//チュートリアルコントローラーの初期化
	m_pTutorialController = std::make_shared<TutorialController>(m_pPlayer, m_controller);
}

void GameScene::Update()
{
	//フレームカウンターの更新
	m_frame++;

	//全GameObjectのUpdateを呼ぶ
	GameObjectManager::GetInstance().UpdateAll();

	//エフェクトマネージャーの更新
	m_pEffectManager->Update();

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

	//サウンドマネージャーの更新
	m_pSoundManager->Update();

	//プレイヤーが特定のz座標まで到達したら
	//カメラを揺らしてボスを登場させる
	if (m_pPlayer->GetPos().z > boss_appear_z)
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
				m_pSoundManager->Stop(SoundManager::SoundType::GameBGM);
				m_bossApearState = BossApearState::Start;
				break;
			case BossApearState::Start:
				//カメラを揺らす(地震のように)
				m_pCamera->OnShake(boss_appear_shake_power, boss_appear_shake_frame);
				//地震音を鳴らす
				m_pSoundManager->Play(SoundManager::SoundType::BossQuake);
				//揺れている間WARNINGを出す
				m_pWarningUI->Start(boss_appear_shake_frame);
				//WARNINGステートに遷移
				m_bossApearState = BossApearState::Warning;
				break;

			case BossApearState::Warning:
				//揺れとWARNINGが終わったらムービーを再生する
				if (!m_pCamera->IsShake() && !m_pWarningUI->IsPlaying())
				{
					int movieH = ResourceLoader::GetInstance().GetGraphic(
						ResourceLoader::GraphicID::BossAppearMovie);
					//リトライで2回目以降に再生することもあるので、頭に戻してから再生する
					SeekMovieToGraph(movieH, 0);
					PlayMovieToGraph(movieH);
					m_isDrawBossMovie = true;
					//ムービーステートに遷移
					m_bossApearState = BossApearState::Movie;
				}
				break;

			case BossApearState::Movie:
			{
				//ムービーの再生が終わったら
				int movieH = ResourceLoader::GetInstance().GetGraphic(
					ResourceLoader::GraphicID::BossAppearMovie);
				if (GetMovieStateToGraph(movieH) == 0)
				{
					//ムービーの最後と同じく、ボスを海面に立たせる
					Position3 bossPos = m_pBoss->GetPos();
					bossPos.y = boss_stand_y;
					m_pBoss->SetPos(bossPos);
					//地震音が鳴っていればフェードアウトする
					m_pSoundManager->FadeOut(SoundManager::SoundType::BossQuake, boss_quake_fade_out_time);
					//ステートをカメラズームに遷移
					m_bossApearState = BossApearState::CameraZoom;
				}
			}
			break;

			case BossApearState::CameraZoom:
				//カメラが揺れていないのを確認してから
				//カメラをムービーの最後と同じ位置へ移す
				//(ズームが終わるとプレイヤー追従に戻るので、そのままプレイヤーの位置へ引いて戻る)
				if (!m_pCamera->IsShake())
				{
					m_pCamera->OnZoomUp(
						boss_appear_zoom_speed,
						m_pBoss,
						boss_appear_camera_dist,
						boss_appear_camera_height);
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

		//カメラがムービーの最後の位置へ移り終わったら、ムービーの描画をやめる
		if (m_isDrawBossMovie && m_isApearBoss && !m_pCamera->IsZoom())
		{
			m_isDrawBossMovie = false;
		}

		//ボスが出現していて、まだボスBGMに切り替えていない場合
		//カメラのズームが終わったタイミングでボスBGMに切り替える
		if (m_isApearBoss && !m_isChangedToBossBGM && !m_pCamera->IsZoom())
		{
			m_pSoundManager->Play(SoundManager::SoundType::BossBGM, true);
			m_isChangedToBossBGM = true;
		}

		//ボスが死亡待機状態になった瞬間、一度だけBGMをフェードアウトする
		if (m_pBoss->IsDying() && !m_isBossDeathBGMFadeOut)
		{
			m_pSoundManager->FadeOut(SoundManager::SoundType::BossBGM, boss_death_bgm_fade_out_time);
			m_isBossDeathBGMFadeOut = true;
		}
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

	//プレイヤーが死亡したらゲームオーバーにする
	if (m_pPlayer->IsDead())
	{
		m_controller.ChangeScene(std::make_shared<GameoverScene>(m_controller), frame_per_second);
	}

	//pauseをsceneに上乗せする
	//ムービーはポーズ中も再生が進んでしまうので、ムービー中はポーズさせない
	if (InputManager::GetInstance().IsTriggered(InputEvent::pause) &&
		m_bossApearState != BossApearState::Movie)
	{
		//ポーズを開くときも決定音を鳴らす
		m_pSoundManager->Play(SoundManager::SoundType::Decision);
		m_controller.PushScene(std::make_shared<PauseScene>(m_controller, m_pSoundManager, m_pPlayer));
	}

	//チュートリアルコントローラーの更新
	m_pTutorialController->Update();
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
	if (DebugManager::GetInstance().IsDebugDrawEnabled())
	{
		DrawString(0, 0, L"GameScene", 0xffffff);
		DrawFormatString(0, 16, 0xffffff, L"FRAME:%d", m_frame);
	}
#endif //DEBUG

	//全GameObjectのDrawを呼ぶ
	GameObjectManager::GetInstance().DrawAll();

	//水マネージャーの描画
	m_pWaterManager->Draw();

	
	//レティクルよりプレイヤーが優先的に描画されてほしいので
	//プレイヤーをもう一度描画する
	m_pPlayer->Draw();
	
	//エフェクトの描画
	m_pEffectManager->Draw();
	
	//全てのUIを描画する
	m_pUIManager->Draw();

	//ボス登場ムービーの再生中は、画面全体にムービーを描画する
	if (m_isDrawBossMovie)
	{
		const Size& wsize = Application::GetInstance().GetWindowSize();
		DrawExtendGraph(0, 0, wsize.width, wsize.height,
			ResourceLoader::GetInstance().GetGraphic(ResourceLoader::GraphicID::BossAppearMovie),
			FALSE);
	}
}

void GameScene::DrawGrid()
{
#ifdef _DEBUG
	if (!DebugManager::GetInstance().IsDebugDrawEnabled()) return;

	//直線の始点と終点
	VECTOR startPos;
	VECTOR endPos;

	//ステージのサイズに合わせてグリッドを描画する
	for (int z = static_cast<int>(-grid_size.z);
		z <= static_cast<int>(grid_size.z); z += 100)
	{
		startPos = VGet(-grid_size.x, 0.0f, static_cast<float>(z));
		endPos = VGet(grid_size.x, 0.0f, static_cast<float>(z));
		DrawLine3D(startPos, endPos, 0xff0000);
	}
	for (int x = static_cast<int>(-grid_size.x); x <= static_cast<int>(grid_size.x); x += 100)
	{
		startPos = VGet(static_cast<float>(x), 0.0f, -grid_size.z);
		endPos = VGet(static_cast<float>(x), 0.0f, grid_size.z);
		DrawLine3D(startPos, endPos, 0x0000ff);
	}
#endif
}

SceneID GameScene::GetSceneID() const
{
	return SceneID::Game;
}