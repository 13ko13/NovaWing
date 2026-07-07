#pragma once
#include <vector>
#include <memory>
#include "Scene.h"

//前方宣言
class Player;
class CameraBase;
class BulletManager;
class FloatingEnemy;
class CollisionManager;
class WormEnemy;
class TargetManager;
class UIManager;
class WaterManager;
class SkyBox;
class GameScene :public Scene
{
public:
	GameScene(SceneController& controller);
	~GameScene();

	void Init() override;
	void Update() override;
	void Draw() override;

private:
	/// <summary>
	/// デバッグ用のグリッドを描画する
	/// </summary>
	void DrawGrid();

private:
	//フレームカウンター
	int m_frame;

	//プレイヤーの実体
	std::shared_ptr<Player> m_pPlayer;
	//カメラの実体
	std::shared_ptr<CameraBase> m_pCamera;
	//浮遊エネミーの実体
	std::shared_ptr<FloatingEnemy> m_pFloatingEnemy;
	//バレットマネージャーの実体
	std::shared_ptr<BulletManager> m_pBulletManager;
	//衝突判定マネージャーの実体
	std::shared_ptr<CollisionManager> m_pCollisionManager;
	//ワームエネミーの実体
	std::shared_ptr<WormEnemy> m_pWormEnemy;
	//ターゲットマネージャーの実体
	std::shared_ptr<TargetManager> m_pTargetManager;
	//UIマネージャー
	std::shared_ptr<UIManager> m_pUIManager;
	//海用の水マネージャー
	std::shared_ptr<WaterManager> m_pWaterManager;
	//スカイボックス
	std::shared_ptr<SkyBox> m_pSkyBox;
};