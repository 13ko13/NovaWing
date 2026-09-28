#include <cassert>

#include "RockDataSetter.h"
#include "Manager/CSVDataLoader.h"
#include "CSVData/CSVData.h"
#include "Utility/Vector3.h"
#include "Game/GameObjects/Actors/Rock/Rock.h"

namespace
{
    const wchar_t* const rock_csv_name = L"RockData";//CSVの名前
    const int model_id_number = 0;//モデルのIDの要素番号
    const int model_x_number = 1;//モデルのX位置要素番号
    const int model_y_number = 2;//モデルのY位置要素番号
    const int model_z_number = 3;//モデルのZ位置要素番号
    const int sphere_radius1 = 4;//1つ目球の半径要素番号
    const int sphere_offset1 = 5;//1つ目球のYオフセット位置要素番号
    const int sphere_radius2 = 6;//2つ目球の半径要素番号
    const int sphere_offset2 = 7;//2つ目球のYオフセット位置要素番号
    const int sphere_radius3 = 8;//3つ目球の半径要素番号
    const int sphere_offset3 = 9;//3つ目球のYオフセット位置要素番号

    //球の数
    constexpr int sphere_num = 3;
    constexpr int rock1_sphere_num = 1;
    //何番目の球か
    constexpr int first_sphere = 0;
    constexpr int second_sphere = 1;
    constexpr int third_sphere = 2;
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
    for(std::shared_ptr<CSVData> data : pData)
    {
        //CSVから読み取った値を受け取る
        dataString = data->GetData();
        
        //モデルID(文字列)をModelIDに変換
        ResourceLoader::ModelID modelID =
            ResourceLoader::WStringToModelID(dataString[model_id_number]);

        //位置(文字列)をVector3に変換
        Vector3 pos = Vector3::FromWString(
            dataString[model_x_number],
            dataString[model_y_number],
            dataString[model_z_number]
        );

        //半径とYオフセット
        std::vector<float> radii;
        std::vector<float> yOffsets;

        //モデルが岩1なら球は一つなので最初の2列しか読み込まない(半径とYオフセット)
        if (modelID == ResourceLoader::ModelID::Rock1)
        {
            radii.resize(rock1_sphere_num);
            yOffsets.resize(rock1_sphere_num);

            radii[first_sphere] = std::stof(dataString[sphere_radius1]);//半径
            yOffsets[first_sphere] = std::stof(dataString[sphere_offset1]);//Yオフセット
        }
        //他の二つのモデルは球が3つないと当たり判定を再現できないので6列読み込む
        else
        {
             radii.resize(sphere_num);
            yOffsets.resize(sphere_num);

            //1つ目の球
            radii[first_sphere] = std::stof(dataString[sphere_radius1]);//半径
            yOffsets[first_sphere] = std::stof(dataString[sphere_offset1]);//Yオフセット
            //2つ目の球
            radii[second_sphere] = std::stof(dataString[sphere_radius2]);//半径
            yOffsets[second_sphere] = std::stof(dataString[sphere_offset2]);//Yオフセット
            //3つ目の球
            radii[third_sphere] = std::stof(dataString[sphere_radius3]);//半径
            yOffsets[third_sphere] = std::stof(dataString[sphere_offset3]);//Yオフセット
        }

        //岩のデータ構造体にいれる
        Rock::RockData data;
        data.modelId = modelID;
        data.pos = pos;
        data.sphereRadii = radii;
        data.sphereYOffsets = yOffsets;

        //その位置と、pCameraで岩を一つ作成する
        pRocks.push_back(std::make_shared<Rock>(pCamera, data));
    }

    return pRocks; 
}
