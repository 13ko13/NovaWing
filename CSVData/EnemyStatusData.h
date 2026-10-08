#pragma once
#include <memory>
#include <unordered_map>

#include "CSVData.h"
#include "Manager/ResourceLoader.h"

class EnemyStatusData : public CSVData
{
public:
	EnemyStatusData(std::shared_ptr<CSVData> data);
	virtual ~EnemyStatusData();

	//各メンバーのゲッター
	ResourceLoader::ModelID GetModelID() const { return m_modelID; }
	int GetHp() const { return m_hp; }
	float GetColRadius() const { return m_colRadius; }
	int GetTrueDeadFrame() const { return m_trueDeadFrame; }
	int GetDeathEffectInterval() const { return m_deathEffectInterval; }

	//ModelIDをキーにエネミーのステータスデータを返す
	static const EnemyStatusData& FindByModelID(ResourceLoader::ModelID id);

private:
	//ヘッダ名で探して、変換
	void Conversion() override;

	//CSVを読んで、ModelIDがキーの表を作る
	static std::unordered_map<ResourceLoader::ModelID, EnemyStatusData> CreateStatusTable();

private:
	ResourceLoader::ModelID m_modelID = ResourceLoader::ModelID::None;
	int m_hp = 0;
	float m_colRadius = 0.0f;
	int m_trueDeadFrame = 0;
	int m_deathEffectInterval = 0;
};
