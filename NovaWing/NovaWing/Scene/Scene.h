#pragma once
#include <vector>

class InputManager;
class SceneController;	//シーンコントローラーのプロトタイプ宣言
class PlayerBullet;

/// <summary>
/// シーン基底クラス
/// </summary>
class Scene
{
protected:
	SceneController& m_controller;

public:
	Scene(SceneController& controller);

	virtual void Init() abstract;

	/// <summary>
	/// シーンの更新処理
	/// </summary>
	virtual void Update() abstract;

	/// <summary>
	/// シーンの描画
	/// </summary>
	virtual void Draw() abstract;
};