#include <memory>
#include <vector>
#include <string> 

#include "WormEnemyDataSetter.h"
#include "Manager/ResourceLoader.h"
#include "Manager/CSVDataLoader.h"
#include "CSVData/CSVData.h"
#include "Utility/Vector3.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/WormEnemy/WormEnemy.h"

namespace
{
    //CSVファイル名
    const wchar_t* const worm_csv_name = L"WormEnemyData";
}

std::vector<std::shared_ptr<WormEnemy>> WormEnemyDataSetter::CreateEnemy(
    std::weak_ptr<Player> pPlayer,
     std::weak_ptr<CameraBase> pCamera,
      std::shared_ptr<BulletManager> pBulletManager)
{
    //最終的な返り値
    std::vector<std::shared_ptr<WormEnemy>> pEnemies;

    //CSVLoaderにCSVをロードさせる
    CSVDataLoader& loader = CSVDataLoader::GetInstance();

    //CSVデータのリストを受け取る
    std::vector<std::shared_ptr<CSVData>> pData = loader.LoadCSV(worm_csv_name);

    //受け取ったCSVの文字列を入れる変数
    std::vector<std::wstring> dataString;
    //リストをループしてそれぞれのデータを受け取る
    for (std::shared_ptr<CSVData> data : pData)
    {
        //CSVから読み取った値を受け取る
        dataString = data->GetData();

        //モデルID(文字列)をModelIDに変換
        ResourceLoader::ModelID modelID =
            ResourceLoader::WStringToModelID(dataString[0]);

        //位置(文字列)をVector3に変換
        Vector3 pos = Vector3::FromWString(
            dataString[1], dataString[2], dataString[3]);

        //胴体の数
        int segmentNum = std::stoi(dataString[4]);

        //移動方向(Z+:1,Z-:-1)
        float direction = std::stof(dataString[5]);

        //その位置と、pCameraで敵を一つ作成する
        pEnemies.push_back(std::make_shared<WormEnemy>(
            pPlayer,//プレイヤー
            modelID,//モデルID
            pBulletManager,//バレットマネージャー
            segmentNum,//胴体の数
            pCamera,//カメラ
            pos,//位置
            direction//移動方向
            ));
    }

    return pEnemies;
}
