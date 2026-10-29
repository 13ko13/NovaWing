#pragma once
#include <vector>
#include <string>
#include <memory>

#include "Game/GameObjects/Actors/Character/Enemy/EnemyFactory.h"
#include "Utility/Vector3.h"

class Player;
class EnemySpawner
{
public:
	struct SpawnData
	{
		EnemyFactory::EnemyType type;//敵の種類
		float activatePlayerZ;//プレイヤーがこのZまで来たら出現
		Vector3 offset;//プレイヤーから見た待機位置のずれ
	};

	EnemySpawner(
		std::weak_ptr<Player> pPlayer,
		std::weak_ptr<EnemyFactory> pEnemyFactory
	);
	virtual ~EnemySpawner();

	//CSVを読み込んで出現予定を作る(offsetZはチュートリアル分のずれ)
	void LoadRushEnemy(const std::wstring& csvFolderName, float offsetZ);
	//プレイヤーが来た予定の敵を作る
	void Update();

private:
	//出現予定情報
	std::vector<SpawnData> m_spawnDatas;
	//プレイヤーへのポインタ
	std::weak_ptr<Player> m_pPlayer;
	//敵生産工場へのポインタ
	std::weak_ptr<EnemyFactory> m_pEnemyFactory;
};
