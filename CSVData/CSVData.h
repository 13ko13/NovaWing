#pragma once
#include <vector>
#include <string>

class CSVData
{
public:
	CSVData();
	virtual ~CSVData();
	//データをセット
	void SetData(std::vector<std::wstring> data) { m_data = data; }
	//ヘッダーをセット
	void SetHeader(std::vector<std::wstring> header) { m_headers = header; }

	//データを取得
	std::vector<std::wstring> GetData() const { return m_data; }
	//ヘッダーを取得
	std::vector<std::wstring> GetHeader() const { return m_headers; }

private:
	//データを変換
	virtual void Conversion() {};
protected:
	std::vector<std::wstring> m_data;
	//ヘッダー
	std::vector<std::wstring> m_headers;

protected:
	//探したい列名の名前を渡して、その列のインデックスを返す
	int GetColumnIndex(const std::wstring& name) const;
};

