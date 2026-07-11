#pragma once
#include <vector>
#include <DxLib.h>
#include <string>
#include <memory>

class CSVData;
/// <summary>
/// CSVファイルをロードするシングルトンクラス
/// </summary>
class CSVDataLoader
{
private:
	//シングルトンの準備
	CSVDataLoader() = default;
	~CSVDataLoader() = default;
	//コピー禁止
	CSVDataLoader(const CSVDataLoader&) = delete;
	CSVDataLoader& operator=(const CSVDataLoader&) = delete;
	//ムーブ禁止(代入)
	CSVDataLoader(CSVDataLoader&&) = delete;
	CSVDataLoader& operator=(CSVDataLoader&&) = delete;
public:
	/// <summary>
	/// シングルトンオブジェクトを返す関数
	/// 参照を返すことで、オブジェクトのコピーを防止する
	/// 間違えて値を返した場合、コピーコンストラクタが走るので
	/// それをprivateにしておくことで、コンパイルエラーにすることができる
	/// </summary>
	/// <returns></returns>
	static CSVDataLoader& GetInstance();

	std::vector<std::shared_ptr<CSVData>> LoadCSV(const wchar_t* path);
private:
	std::vector<std::vector<std::wstring>> GetWStringList(const wchar_t* path);
};

