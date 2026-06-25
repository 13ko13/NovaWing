#pragma once
#include <vector>
#include <memory>

class GameObject;
class GameObjectManager
{
public:
	/// <summary>
	/// GameObjectManagerのインスタンスを取得する
	/// </summary>
	/// <returns></returns>
	static GameObjectManager& GetInstance();

	//コピーと代入を禁止する(消す)
	GameObjectManager(const GameObjectManager&) = delete;
	GameObjectManager& operator=(const GameObjectManager&) = delete;

	/// <summary>
	/// GameObjectを継承しているクラスを登録する
	/// </summary>
	void Register(std::shared_ptr<GameObject> pGameObject);

	/// <summary>
	/// 全GameObjectのUpdateを呼ぶ
	/// </summary>
	void UpdateAll();

	/// <summary>
	/// 全GameObjectのDrawを呼ぶ
	/// </summary>
	void DrawAll();

private:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	GameObjectManager();

	/// <summary>
	/// 毎フレーム条件が満たされたGameObjectを削除する
	/// </summary>
	void RemoveGameObject();

private:
	//ゲームオブジェクトの配列
	std::vector<std::shared_ptr<GameObject>> m_pGameObjects;
	//保留中のゲームオブジェクトの配列
	std::vector<std::shared_ptr<GameObject>> m_pHoldGameObject;
};

