#include <cassert>

#include "RockDataSetter.h"
#include "Manager/CSVDataLoader.h"
#include "CSVData/CSVData.h"
#include "Utility/Vector3.h"
#include "Game/GameObjects/Actors/Rock/Rock.h"

namespace
{
    const wchar_t* const rock_csv_name = L"RockData";
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
        ResourceLoader::ModelID modelID = WStringToModelID(dataString[0]);

        //位置(文字列)をVector3に変換
        Vector3 pos = Vector3::FromWString(
            dataString[1],dataString[2],dataString[3]);

        //その位置と、pCameraで岩を一つ作成する
        pRocks.push_back(std::make_shared<Rock>(modelID,pCamera,pos));
    }

    return pRocks; 
}

ResourceLoader::ModelID RockDataSetter::WStringToModelID(const std::wstring id)
{
    //渡された文字列に応じて岩のモデルを返す
    if(id == L"Rock1")
    {
        return ResourceLoader::ModelID::Rock1;
    }
    else if(id == L"Rock2")
    {
        return ResourceLoader::ModelID::Rock2;
    }
    else if(id == L"Rock3")
    {
        return ResourceLoader::ModelID::Rock3;
    }
    else
    {
        //それ以外の文字列ならクラッシュ
        assert(-1 + L"そのモデルIDはありません");
        //Release版では異常事態なので1番を返しておく
        return ResourceLoader::ModelID::Rock1;
    }
}
