#include <cassert>

#include "CSVData.h"

CSVData::CSVData():
	m_data()
{
}

CSVData::~CSVData()
{
}

int CSVData::GetColumnIndex(const std::wstring& name) const
{
	//ヘッダを先頭から順にみて、nameと一致するものを返す
	for(size_t i = 0; i < m_headers.size(); i++)
	{
		//ヘッダーとnameが一致しているならiを返す
		if (m_headers[i] == name) return static_cast<int>(i);
	}

	//最後まで見つからなかったらassert
	assert(false && "CSVに指定した列名がありません");

	return -1;
}
