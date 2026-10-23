#pragma once
#include <vector>
#include <memory>
#include "Scene.h"

//前方宣言
class Player;
class GameCamera;
class BulletManager;
class EnemyBase;
class CollisionManager;
class TargetManager;
class UIManager;
class WaterManager;
class SkyBox;
class Rock;
class Scenery;
class Stage;
class BossEnemy;
class EnemyFactory;
class SoundManager;
class EffectManager;
class WarningUI;
class TutorialController;
class StageConfig;
class GameScene :public Scene
{
public:
	GameScene(SceneController& controller, int stageNo);
	~GameScene();

	void Init() override;
	void Update() override;
	void Draw() override;
	SceneID GetSceneID() const override;//自身のシーンIDを返す

#ifdef _DEBUG
	//デバッグ用:プレイヤーを通常ステージの指定Z座標(ステージ開始位置からの距離)へ移動する
	//チュートリアル中ならチュートリアルをスキップして、その場から通常ステージを始める
	void WarpPlayer(float stageZ);
#endif

private:
#ifdef _DEBUG
	/// <summary>
	/// デバッグ用ウィンドウ(DebugUI)に、このシーンの操作項目を登録する
	/// </summary>
	void RegisterDebugItems();

	//チュートリアルが終わったかを返す
	bool IsFinishedTutorial() const;

#endif

	/// <summary>
	/// デバッグ用のグリッドを描画する
	/// </summary>
	void DrawGrid();

	//チュートリアル終了を検知して、終了していたら
	//エネミーや岩を出現させる
	//プレイヤーがチュートリアルを終了したZ座標を渡す必要がある
	void CreateStageObjects(float offsetZ);

	//プレイヤーの今の位置を基準に通常ステージを始める
	void StartStage();

	//ゲーム中の更新
	void UpdateGameWorld();
private:
	//フレームカウンター
	int m_frame;

	//ボスが出現したか
	bool m_isAppearBoss = false;
	//ボスBGMに切り替え済みか
	bool m_isChangedToBossBGM = false;
	//ボス死亡時のBGMフェードアウト済みか
	bool m_isBossDeathBGMFadeOut = false;
	//ボス登場ムービーを描画するか
	//(ムービー終了後もカメラが最後の位置へ移り終わるまでは最後のコマを出し続ける)
	bool m_isDrawBossMovie = false;

	//敵を出現させたか
	bool m_isAppearEnemy = false;

	//ボス登場時の演出ステート
	enum class BossApearState
	{
		None,//通常
		Start,//開始時
		Warning,//揺れている間WARNINGを出す
		Movie,//海から浮上するムービーを再生
		CameraZoom,//カメラをズームアップ
	};
	BossApearState m_bossApearState = BossApearState::None;

	//プレイヤーがチュートリアルを終了した位置を保存しておく(敵が出てくる位置のオフセット)
	float m_stageOffsetZ = 0.0f;

	//プレイヤーへのポインタ
	std::shared_ptr<Player> m_pPlayer;
	//カメラへのポインタ
	std::shared_ptr<GameCamera> m_pCamera;
	//敵へのポインタの配列
	std::vector<std::shared_ptr<EnemyBase>> m_pEnemies;
	//バレットマネージャーへのポインタ
	std::shared_ptr<BulletManager> m_pBulletManager;
	//衝突判定マネージャーへのポインタの配列
	std::shared_ptr<CollisionManager> m_pCollisionManager;
	//ターゲットマネージャーへのポインタ
	std::shared_ptr<TargetManager> m_pTargetManager;
	//UIマネージャーへのポインタ
	std::shared_ptr<UIManager> m_pUIManager;
	//海用の水マネージャーへのポインタ
	std::shared_ptr<WaterManager> m_pWaterManager;
	//スカイボックスへのポインタ
	std::shared_ptr<SkyBox> m_pSkyBox;
	//岩へのポインタの配列
	std::vector<std::shared_ptr<Rock>> m_pRocks;
	//景色(島など、ゲームに関与しないオブジェクト)へのポインタの配列
	std::vector<std::shared_ptr<Scenery>> m_pSceneries;
	//ステージへのポインタ
	std::shared_ptr<Stage> m_pStage;
	//ボスへのポインタ
	std::shared_ptr<BossEnemy> m_pBoss;
	//敵生産工場へのポインタ
	std::shared_ptr<EnemyFactory> m_pEnemyFactory;
	//サウンドマネージャーへのポインタ
	std::shared_ptr<SoundManager> m_pSoundManager;
	//エフェクトマネージャーへのポインタ
	std::shared_ptr<EffectManager> m_pEffectManager;
	//ボス登場前のWARNINGへのポインタ
	std::shared_ptr<WarningUI> m_pWarningUI;
	//チュートリアルコントローラーへのポインタ
	std::shared_ptr<TutorialController> m_pTutorialController;
	//ステージ設定用のクラスへのポインタ
	std::unique_ptr<StageConfig> m_pStageConfig;
};