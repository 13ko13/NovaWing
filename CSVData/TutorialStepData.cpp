#include "TutorialStepData.h"

TutorialStepData::TutorialStepData(std::shared_ptr<CSVData> data)
{
	m_data = data->GetData();
	m_headers = data->GetHeader();
	Conversion();
}

TutorialStepData::~TutorialStepData()
{
}

void TutorialStepData::Conversion()
{
	m_stepID = std::stoi(m_data[GetColumnIndex(L"stepID")]);
	m_message = m_data[GetColumnIndex(L"message")];
	m_taskText = m_data[GetColumnIndex(L"taskText")];
	m_taskType = m_data[GetColumnIndex(L"taskType")];
}