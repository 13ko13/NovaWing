#include <cassert>

#include "EnemyStatusData.h"
#include "Manager/CSVDataLoader.h"

EnemyStatusData::EnemyStatusData(std::shared_ptr<CSVData> data)
{
	//ヘッダーとデータをセット
	m_data = data->GetData();
	m_headers = data->GetHeader();

	//その後変換する
	Conversion();
}

EnemyStatusData::~EnemyStatusData()
{
}

const EnemyStatusData& EnemyStatusData::FindByModelID(ResourceLoader::ModelID id)
{
	//モデルIDをキーとしたデータの表を作成
	static const auto table = CreateStatusTable();
	//テーブルにIDが存在するならエネミーのステータスデータを返す
	auto it = table.find(id);
	if (it != table.end())
	{
		return it->second;
	}
	else
	{
		assert(false && "存在しないモデルIDが指定されました");
		return it->second;
	}
}

void EnemyStatusData::Conversion()
{
	//列ごとに文字列を取り出して変換してメンバにいれる
	m_modelID = ResourceLoader::WStringToModelID(m_data[GetColumnIndex(L"modelID")]);
	m_hp = std::stoi(m_data[GetColumnIndex(L"hp")]);
	m_colRadius = std::stof(m_data[GetColumnIndex(L"colRadius")]);
	m_trueDeadFrame = std::stoi(m_data[GetColumnIndex(L"trueDeadFrame")]);
	m_deathEffectInterval = std::stoi(m_data[GetColumnIndex(L"deathEffectInterval")]);
}

std::unordered_map<ResourceLoader::ModelID, EnemyStatusData> EnemyStatusData::CreateStatusTable()
{
	//EnemyStatusを全行読んで、ModelIDで探せる表を作る
	std::unordered_map<ResourceLoader::ModelID, EnemyStatusData> table;
	//データを受け取る
	auto data = CSVDataLoader::GetInstance().LoadCSV(L"Params/EnemyStatus");

	//1行ずつ変換して表にいれる
	for (auto& row : data)
	{
		//rowをコンストラクタに渡して変換させる
		EnemyStatusData status = EnemyStatusData(row);
		//tableにいれる
		table.emplace(status.GetModelID(), status);
	}
	return table;
}
