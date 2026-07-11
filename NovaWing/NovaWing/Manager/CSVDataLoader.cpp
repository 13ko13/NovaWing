#include <fstream> //ファイルの読み込みに必要
#include <sstream> //文字列分解用(stringstream)stringをファイルのように扱える
#include <codecvt>

#include "CSVDataLoader.h"
#include "CSVData/CSVData.h"
#include "Utility/StringUtil.h"

namespace
{
	//使うCSVフォルダのパス
	const std::wstring csv_path = L"Data/CSV/";
	//CSVファイル拡張子
	const std::wstring csv = L".csv";
}

CSVDataLoader& CSVDataLoader::GetInstance()
{
	//この時点でメモリが確保されてアプリ終了まで残る
	static CSVDataLoader instance;
	return instance;
}

std::vector<std::shared_ptr<CSVData>> CSVDataLoader::LoadCSV(const wchar_t* path)
{
	//csvから読み取ったデータを格納する配列
	std::vector<std::shared_ptr<CSVData>> datas;

	//パス
	std::wstring pPath = csv_path + path + csv;
	//データをすべて読み込む
	auto valuesDatas = GetWStringList(pPath.c_str());

	for (auto values : valuesDatas)
	{
		//データ
		std::shared_ptr<CSVData> data = std::make_shared<CSVData>();
		//データをセット
		data->SetData(values);
		//配列に追加
		datas.push_back(data);
	}
	return datas;
}

std::vector<std::vector<std::wstring>> CSVDataLoader::GetWStringList(const wchar_t* path)
{
	std::vector<std::vector<std::wstring>> valuesDatas;
	//ファイルを開く
	std::ifstream file(path);

	if (!file.is_open())
	{
		//ファイルが開けなかったときは空の配列を返す
		return valuesDatas;
	}

	//ファイル内容を一括で読み込み(UTF-8)
	std::string utf8Data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	//ファイルが空の場合は空のリストを返す
	if (utf8Data.empty())
	{
		return valuesDatas;
	}

	//UTF-8からUTF-16に変換
	int wideSize = MultiByteToWideChar(
		CP_UTF8,
		0,
		utf8Data.c_str(),
		static_cast<int>(utf8Data.size()),
		nullptr,
		0
	);
	//wideSize文字分の領域を確保して、全て0で初期化する
	std::wstring wdata(wideSize, 0);
	MultiByteToWideChar(
		CP_UTF8,
		0,
		utf8Data.c_str(),
		static_cast<int>(utf8Data.size()),
		&wdata[0],
		wideSize
	);

	//行ごとに処理
	std::wstringstream ws(wdata);
	std::wstring line;
	bool isHeader = false;//ヘッダー行を読み飛ばすためのフラグ
	std::vector<std::wstring> header;//ヘッダー行を格納する配列

	//CSVの行を終わりまで読み取る
	//getlineで1行ずつ読み取る
	while (std::getline(ws, line))
	{
		//行をカンマ切りで1つずつ分解するする準備
		std::wstringstream lineStream(line);
		std::wstring part;
		std::vector<std::wstring> values;

		//カンマ区切りで分解してリストに追加
		while(std::getline(lineStream,part,L','))
		{
			//改行処理(findで探してnposが返ってこなければ改行を挟む)
			for (size_t i = 0; (i = part.find(L"\\n", i)) != std::wstring::npos; i++)
			{
				part.replace(i, 2, L"\n");
			}

			values.emplace_back(part);
		}

		//ヘッダー
		if (!isHeader)
		{
			header = values;
			isHeader = true;
			continue;
		}

		//要素数チェック(ヘッダーより少ない場合は不正な行としてスキップ)
		if (values.size() < header.size()) continue;

		//データを配列に追加
		valuesDatas.emplace_back(values);
	}

	//暗黙ムーブが走るのでコピーコストは問題なし
	return valuesDatas;
}