#include <string>

#include "EnemySpawner.h"
#include "Manager/CSVDataLoader.h"
#include "CSVData/CSVData.h"
#include "Game/GameObjects/Actors/Character/Player/Player.h"
#include "Game/GameObjects/Actors/Character/Enemy/EnemyFactory.h"

namespace
{
	//CSVファイル名
	constexpr const wchar_t* rush_csv_name = L"RushEnemy";
	//突進する敵がプレイヤーのどれだけ前で待機するかのCSV列番号
	constexpr int front_offset_number = 10;
}

EnemySpawner::EnemySpawner(
	std::weak_ptr<Player> pPlayer,
	std::weak_ptr<EnemyFactory> pEnemyFactory) :
	m_pPlayer(pPlayer),
	m_pEnemyFactory(pEnemyFactory)
{
}

EnemySpawner::~EnemySpawner()
{
}

void EnemySpawner::LoadRushEnemy(const std::wstring& csvFolderName, float offsetZ)
{
	//CSVLoaderにCSVをロードさせる
	CSVDataLoader& loader = CSVDataLoader::GetInstance();

	//CSVデータのリストを受け取る
	std::wstring csvPath = csvFolderName + L"/" + rush_csv_name;
	std::vector<std::shared_ptr<CSVData>> pData = loader.LoadCSV(csvPath.c_str());

	//受け取ったCSVの文字列を入れる変数
	std::vector<std::wstring> dataString;
	//リストをループしてそれぞれのデータを受け取る
	for (std::shared_ptr<CSVData> data : pData)
	{
		//CSVから読み取った値を受け取る
		dataString = data->GetData();

		//位置(文字列)をVector3に変換
		Vector3 pos = Vector3::FromWString(
			dataString[1], dataString[2], dataString[3]);

		//プレイヤーがチュートリアルを終了した地点分ずらす
		pos.z += offsetZ;

		//プレイヤーのどれだけ前で待機するかの値を受け取る
		float frontOffset = std::stof(dataString[front_offset_number]);

		//出現予定を作成する
		SpawnData spawnData;
		spawnData.type = EnemyFactory::EnemyType::RushEnemy;//敵の種類
		spawnData.activatePlayerZ = pos.z;//プレイヤーがこのZまで来たら出現
		spawnData.offset = Vector3(pos.x, pos.y, frontOffset);//待機位置をどのぐらいずらすか
		m_spawnDatas.push_back(spawnData);
	}
}

void EnemySpawner::Update()
{
	//プレイヤーの位置を取得
	Vector3 playerPos = m_pPlayer.lock()->GetPos();

	//出現予定の敵を出現させる
	auto it = m_spawnDatas.begin();
	while (it != m_spawnDatas.end())
	{
		//プレイヤーが出現予定の敵のZより進んでいたら出現させる
		if (playerPos.z > it->activatePlayerZ)
		{
			m_pEnemyFactory.lock()->Create(playerPos + it->offset, it->type);
			//もう出現させたので配列から削除する
			//eraseが次の要素を返してくれる
			it = m_spawnDatas.erase(it);
		}
		else
		{
			//まだついていないなら次へ
			it++;
		}
	}
	
}
