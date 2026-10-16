#include "TutorialEnemyData.h"

TutorialEnemyData::TutorialEnemyData(std::shared_ptr<CSVData> data)
{
	m_data = data->GetData();
	m_headers = data->GetHeader();
	Conversion();
}

TutorialEnemyData::~TutorialEnemyData()
{
}

Vector3 TutorialEnemyData::GetOffset() const
{
	return m_offset;
}

void TutorialEnemyData::Conversion()
{
	//列名で取り出して変換する(stepIDはstoi、offsetX/Y/Zはstof、enemyTypeはそのまま)
	m_stepID = std::stoi(m_data[GetColumnIndex(L"stepID")]);
	m_offset = Vector3(
		std::stof(m_data[GetColumnIndex(L"offsetX")]),
		std::stof(m_data[GetColumnIndex(L"offsetY")]),
		std::stof(m_data[GetColumnIndex(L"offsetZ")])
	);
	//エネミータイプを受け取る
	m_enemyType = m_data[GetColumnIndex(L"enemyType")];
}
