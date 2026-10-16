#pragma once

#include "CSVData.h"

#include <string>
#include <memory>

class TutorialStepData : public CSVData {
public:
	TutorialStepData(std::shared_ptr<CSVData> data);
	virtual ~TutorialStepData();

	/// ステップ番号取得
	int GetStepID() const { return m_stepID; }
	const std::wstring& GetMessageText() const { return m_message; }
	const std::wstring& GetTaskText() const{ return m_taskText; }
	const std::wstring& GetTaskType() const{ return m_taskType; }

private:
	//変換	
	void Conversion() override;

private:
	int m_stepID = 0;
	std::wstring m_message;
	std::wstring m_taskText;
	std::wstring m_taskType;
};
