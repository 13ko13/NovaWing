#include "BossEnemyDataSetter.h"
#include "Manager/ResourceLoader.h"
#include "Manager/CSVDataLoader.h"
#include "CSVData/CSVData.h"
#include "Utility/Vector3.h"
#include "BossEnemy.h"

namespace
{
    //CSVファイル名
    const wchar_t* const boss_csv_name = L"BossEnemyData";
}

std::shared_ptr<BossEnemy> BossEnemyDataSetter::CreateEnemy(
        std::weak_ptr<Player> pPlayer,
        std::weak_ptr<CameraBase> pCamera,
        std::weak_ptr<BulletManager> pBulletManager,
        std::weak_ptr<EnemyFactory> pEnemyFactory
)
{
    std::shared_ptr<BossEnemy> pEnemy;

    //CSVLoaderにCSVをロードさせる
    CSVDataLoader& loader = CSVDataLoader::GetInstance();

    //CSVデータのリストを受け取る
    std::vector<std::shared_ptr<CSVData>> pData = loader.LoadCSV(boss_csv_name);

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

        BossEnemy::BossEnemyData bossData;
        bossData.Id = modelID;
        bossData.pBulletManager = pBulletManager;
        bossData.pCamera = pCamera;
        bossData.pEnemyFactory = pEnemyFactory;
        bossData.pos = pos;
        bossData.pPlayer = pPlayer;

        //その位置と、pCameraで敵を一つ作成する
        pEnemy = std::make_shared<BossEnemy>(bossData);
    }
    //出来上がったボスのポインタを返す
    return pEnemy;
}