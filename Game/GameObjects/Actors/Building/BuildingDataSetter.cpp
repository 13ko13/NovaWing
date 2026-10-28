#include <map>

#include "BuildingDataSetter.h"
#include "Manager/ResourceLoader.h"
#include "Building.h"
#include "Manager/CSVDataLoader.h"
#include "CSVData/CSVData.h"

namespace
{
	//csvの名前
	constexpr const wchar_t* building_csv_name = L"Building";
	constexpr int model_id_number = 0;//モデルのIDの要素番号
    constexpr int model_x_number = 1;//モデルのX位置要素番号
    constexpr int model_y_number = 2;//モデルのY位置要素番号
	constexpr int model_z_number = 3;//モデルのZ位置要素番号
	constexpr int model_rot_y_number = 5;//モデルのY軸回転(度)の要素番号
	constexpr int model_scale_x_number = 7;//モデルのXスケールの要素番号
	constexpr int model_scale_y_number = 8;//モデルのYスケールの要素番号
	constexpr int model_scale_z_number = 9;//モデルのZスケールの要素番号
	
	//モデルIDをキーとした当たり判定のモデルID情報
	const std::map<ResourceLoader::ModelID, ResourceLoader::ModelID> col_model_ids =
	{
		//倒壊している高いビル
		{ ResourceLoader::ModelID::BuildingTallBroken,ResourceLoader::ModelID::BuildingTallBrokenCol },
		//アンテナがついているビル
		{ ResourceLoader::ModelID::BuildingAntennaTower,ResourceLoader::ModelID::BuildingAntennaTowerCol },
		//橋がついているビル
		{ ResourceLoader::ModelID::BuildingBridgeWreck,ResourceLoader::ModelID::BuildingBridgeWreckCol },
		//壊れたビル
		{ ResourceLoader::ModelID::BuildingCollapsed,ResourceLoader::ModelID::BuildingCollapsedCol },
		//傾いているビル
		{ ResourceLoader::ModelID::BuildingLeaning,ResourceLoader::ModelID::BuildingLeaningCol },
		//壊れたオフィスがあるビル
		{ ResourceLoader::ModelID::BuildingOfficeGutted,ResourceLoader::ModelID::BuildingOfficeGuttedCol },
	};
}

BuildingDataSetter::BuildingDataSetter()
{
}

BuildingDataSetter::~BuildingDataSetter()
{
}

std::vector<std::shared_ptr<Building>> BuildingDataSetter::CreateBuilding(
	std::weak_ptr<CameraBase> pCamera,
	float offsetZ,
	const std::wstring& csvFolderName)
{
	//最終的な返り値
    std::vector<std::shared_ptr<Building>> pBuildings;

    //CSVLoaderにCSVをロードさせる
    CSVDataLoader& loader = CSVDataLoader::GetInstance();

	//CSVデータのリストを受け取る
	std::wstring csvPath = csvFolderName + L"/" + building_csv_name;
	std::vector<std::shared_ptr<CSVData>> pData = loader.LoadCSV(csvPath.c_str());

    //受け取ったCSVの文字列を入れる変数
    std::vector<std::wstring> dataString;
    //リストをループしてそれぞれのデータを受け取る
    for(std::shared_ptr<CSVData> buildingData : pData)
    {
        //CSVから読み取った値を受け取る
        dataString = buildingData->GetData();
        
        //モデルID(文字列)をModelIDに変換
        ResourceLoader::ModelID modelID =
            ResourceLoader::WStringToModelID(dataString[model_id_number]);

        //位置(文字列)をVector3に変換
        Vector3 pos = Vector3::FromWString(
            dataString[model_x_number],
            dataString[model_y_number],
            dataString[model_z_number]
		);

		//プレイヤーがチュートリアルを終了した位置分ずらす
		pos.z += offsetZ;

		//Y軸回転量(文字列)をfloatに変換
		float rotYRadian = std::stof(dataString[model_rot_y_number]);
		rotYRadian *= DX_PI_F / 180.0f;

		//モデルの大きさ(文字列)をVector3に変換
		Vector3 scale = Vector3::FromWString(
			dataString[model_scale_x_number],
			dataString[model_scale_y_number],
			dataString[model_scale_z_number]
		);

		//当たり判定用のモデルを受け取る
		ResourceLoader::ModelID colModelID = col_model_ids.at(modelID);

        //ビルのデータ構造体にいれる
        Building::BuildingData data;
        data.modelId = modelID;
        data.pos = pos;
		data.scale = scale;
		data.colModelID = colModelID;
		data.rotYRadian = rotYRadian;

        //カメラとデータをもとにビルを一つ作成する
        pBuildings.push_back(std::make_shared<Building>(pCamera, data));
    }

    return pBuildings; 
}
