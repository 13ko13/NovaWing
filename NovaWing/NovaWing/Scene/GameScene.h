#pragma once
#include <vector>
#include <memory>
#include "../Utility/Vector3.h"
#include "Scene.h"

//前方宣言
class Player;
class CameraBase;
class BulletManager;
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
	int m_frameCount;

	//プレイヤーの実体
	std::shared_ptr<Player> m_pPlayer;

	//カメラの実体
	std::shared_ptr<CameraBase> m_pCamera;

	//バレットマネージャーの実体
	std::shared_ptr<BulletManager> m_pBulletManager;
};