#pragma once
#include <vector>
#include <memory>
#include "../Utility/Vector3.h"
#include "Scene.h"

//前方宣言
class Player;
class InputManager;
class Camera;
class GameScene :public Scene
{
public:
	GameScene(SceneController& controller,InputManager& input);
	~GameScene();

	void Init() override;
	void Update(InputManager& input) override;
	void Draw() override;

private:
	/// <summary>
	/// デバッグ用のグリッドを描画する
	/// </summary>
	void DrawGrid();

	/// <summary>
	/// Actorを継承しているオブジェクトをすべて格納する
	/// </summary>
	void PushAllActor();

private:
	//フレームカウンター
	int m_frameCount;

	//Actorをすべて格納したもの
	std::vector<std::shared_ptr<Actor>> m_pActors;

	//プレイヤーの実体
	std::shared_ptr<Player> m_pPlayer;

	//カメラの実体
	std::shared_ptr<Camera> m_pCamera;

	//バレットマネージャーの実体
	std::shared_ptr
};