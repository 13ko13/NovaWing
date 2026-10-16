#pragma once
#include <string>
#include <memory>

#include "CSVData/CSVData.h"
#include "Utility/Vector3.h"

/// <summary>
/// TutorialEnemy.csvの1行(チュートリアルのステップで出す敵1体)を型に直したデータ
/// </summary>
class TutorialEnemyData : public CSVData
{
public:
	TutorialEnemyData(std::shared_ptr<CSVData> data);
	virtual ~TutorialEnemyData();

	//どのステップの敵か(Tutorial.csvのstepIDと対応)
	int GetStepID() const { return m_stepID; }
	//敵の種類(文字列のまま。enumへの変換はTutorialController側で行う)
	const std::wstring& GetEnemyType() const { return m_enemyType; }
	//プレイヤーの位置からのずれ
	Vector3 GetOffset() const;

private:
	//変換
	void Conversion() override;

private:
	int m_stepID = 0;
	std::wstring m_enemyType;
	//プレイヤーの位置からのずれ
	Vector3 m_offset;
};
