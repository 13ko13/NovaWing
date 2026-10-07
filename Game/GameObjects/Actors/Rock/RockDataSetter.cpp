#include <cassert>
#include <map>

#include "RockDataSetter.h"
#include "Manager/CSVDataLoader.h"
#include "CSVData/CSVData.h"
#include "Utility/Vector3.h"
#include "Game/GameObjects/Actors/Rock/Rock.h"

namespace
{
    const wchar_t* const rock_csv_name = L"Stage1/Rock";//CSVの名前
    const int model_id_number = 0;//モデルのIDの要素番号
    const int model_x_number = 1;//モデルのX位置要素番号
    const int model_y_number = 2;//モデルのY位置要素番号
	const int model_z_number = 3;//モデルのZ位置要素番号
	const int model_rot_y_number = 5;//モデルのY軸回転(度)の要素番号

	//1つのモデル分の球の情報
	struct RockSphereData
	{
		std::vector<float> radii;//半径
		std::vector<float> yOffsets;//Yオフセット
	};
	//モデルIDをキーとした岩の球の情報
	const std::map<ResourceLoader::ModelID, RockSphereData> sphere_settings =
	{
		//Rock1の場合
		{ ResourceLoader::ModelID::Rock1,{ { 128.0f } ,{ 128.0f } } },
		//Rock2の場合
		{ ResourceLoader::ModelID::Rock2,{ { 64.0f,180.0f,128.0f } ,{ 512.0f,256.0f,0.0f } } },
		//Rock3の場合
		{ ResourceLoader::ModelID::Rock3,{ { 90.0f,100.0f,128.0f } ,{ 460.0f,256.0f,0.0f } } },
	};
}

std::vector<std::shared_ptr<Rock>> RockDataSetter::CreateRock(
    std::weak_ptr<CameraBase> pCamera)
{
    //最終的な返り値
    std::vector<std::shared_ptr<Rock>> pRocks;

    //CSVLoaderにCSVをロードさせる
    CSVDataLoader& loader = CSVDataLoader::GetInstance();

    //CSVデータのリストを受け取る
    std::vector<std::shared_ptr<CSVData>> pData = loader.LoadCSV(rock_csv_name);

    //受け取ったCSVの文字列を入れる変数
    std::vector<std::wstring> dataString;
    //リストをループしてそれぞれのデータを受け取る
    for(std::shared_ptr<CSVData> rockData : pData)
    {
        //CSVから読み取った値を受け取る
        dataString = rockData->GetData();
        
        //モデルID(文字列)をModelIDに変換
        ResourceLoader::ModelID modelID =
            ResourceLoader::WStringToModelID(dataString[model_id_number]);

        //位置(文字列)をVector3に変換
        Vector3 pos = Vector3::FromWString(
            dataString[model_x_number],
            dataString[model_y_number],
            dataString[model_z_number]
		);

		//Y軸回転量(文字列)をfloatに変換
		float rotYRadian = std::stof(dataString[model_rot_y_number]);
		rotYRadian *= DX_PI_F / 180.0f;

        //岩のデータ構造体にいれる
        Rock::RockData data;
        data.modelId = modelID;
        data.pos = pos;
        data.sphereRadii = sphere_settings.at(modelID).radii;
		data.sphereYOffsets = sphere_settings.at(modelID).yOffsets;
		data.rotYRadian = rotYRadian;

        //その位置と、pCameraで岩を一つ作成する
        pRocks.push_back(std::make_shared<Rock>(pCamera, data));
    }

    return pRocks; 
}