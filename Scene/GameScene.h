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
class Stage;
class BossEnemy;
class EnemyFactory;
class SoundManager;
class EffectManager;
class WarningUI;
class GameScene :public Scene
{
public:
	GameScene(SceneController& controller);
	~GameScene();

	void Init() override;
	void Update() override;
	void Draw() override;
	SceneID GetSceneID() const override;//自身のシーンIDを返す

private:
	/// <summary>
	/// デバッグ用のグリッドを描画する
	/// </summary>
	void DrawGrid();

private:
	//フレームカウンター
	int m_frame;

	//ボスが出現したか
	bool m_isApearBoss = false;
	//ボスBGMに切り替え済みか
	bool m_isChangedToBossBGM = false;
	//ボス死亡時のBGMフェードアウト済みか
	bool m_isBossDeathBGMFadeOut = false;
	//ボス登場ムービーを描画するか
	//(ムービー終了後もカメラが最後の位置へ移り終わるまでは最後のコマを出し続ける)
	bool m_isDrawBossMovie = false;

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
};