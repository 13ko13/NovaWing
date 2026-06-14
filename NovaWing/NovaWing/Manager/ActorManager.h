#pragma once
#include <vector>
#include <memory>

class Actor;
class ActorManager
{
public:
	/// <summary>
	/// ActorManagerのインスタンスを取得する
	/// </summary>
	/// <returns></returns>
	static ActorManager& GetInstance();

	//コピーと代入を禁止する(消す)
	ActorManager(const ActorManager&) = delete;
	ActorManager& operator=(const ActorManager&) = delete;

	/// <summary>
	/// Actorを継承しているクラスを登録する
	/// </summary>
	void Register(std::shared_ptr<Actor> pActor);

	/// <summary>
	/// 全ActorのUpdateを呼ぶ
	/// </summary>
	void UpdateAll();

	/// <summary>
	/// 全ActorのDrawを呼ぶ
	/// </summary>
	void DrawAll();

private:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	ActorManager();

	/// <summary>
	/// 毎フレーム条件が満たされたActorを削除する
	/// </summary>
	void RemoveActor();

private:
	std::vector<std::shared_ptr<Actor>> m_pActors;
};

