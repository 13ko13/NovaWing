#include <DxLib.h>

#include "SceneryDataSetter.h"
#include "Manager/CSVDataLoader.h"
#include "CSVData/CSVData.h"
#include "Utility/Vector3.h"
#include "Game/GameObjects/Actors/Scenery/Scenery.h"

namespace
{
	const wchar_t* const scenery_csv_name = L"Stage1/Scenery";//CSVの名前
	const int model_id_number = 0;//モデルのIDの要素番号
	const int model_x_number = 1;//モデルのX位置要素番号
	const int model_y_number = 2;//モデルのY位置要素番号
	const int model_z_number = 3;//モデルのZ位置要素番号
	const int model_rot_y_number = 5;//モデルのY軸回転(度)の要素番号
	const int model_scale_x_number = 7;//モデルのXサイズ要素番号
	const int model_scale_y_number = 8;//モデルのYサイズ要素番号
	const int model_scale_z_number = 9;//モデルのZサイズ要素番号
}

std::vector<std::shared_ptr<Scenery>> SceneryDataSetter::CreateScenery(
	std::weak_ptr<CameraBase> pCamera)
{
	//最終的な返り値
	std::vector<std::shared_ptr<Scenery>> pSceneries;

	//CSVDataLoaderにCSVをロードさせる
	CSVDataLoader& loader = CSVDataLoader::GetInstance();
	std::vector<std::shared_ptr<CSVData>> pData = loader.LoadCSV(scenery_csv_name);

	for (std::shared_ptr<CSVData> sceneryData : pData)
	{
		//CSVから読み取った値を受け取る
		std::vector<std::wstring> dataString = sceneryData->GetData();

		Scenery::SceneryData data;
		//モデルID(文字列)をModelIDに変換
		data.modelId = ResourceLoader::WStringToModelID(dataString[model_id_number]);
		//位置(文字列)をVector3に変換
		data.pos = Vector3::FromWString(
			dataString[model_x_number],
			dataString[model_y_number],
			dataString[model_z_number]
		);
		//Y軸回転量(度)をラジアンに変換
		data.rotYRadian = std::stof(dataString[model_rot_y_number]) * DX_PI_F / 180.0f;
		//サイズ(文字列)をVector3に変換
		data.scale = Vector3::FromWString(
			dataString[model_scale_x_number],
			dataString[model_scale_y_number],
			dataString[model_scale_z_number]
		);

		pSceneries.push_back(std::make_shared<Scenery>(pCamera, data));
	}

	return pSceneries;
}
