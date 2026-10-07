#include <memory>
#include <vector>
#include <string> 

#include "WormEnemyDataSetter.h"
#include "Manager/ResourceLoader.h"
#include "Manager/CSVDataLoader.h"
#include "CSVData/CSVData.h"
#include "Utility/Vector3.h"
#include "Game/GameObjects/Actors/Character/Enemy/WormEnemy/WormEnemy.h"
#include "Game/GameObjects/Actors/Character/Enemy/EnemyBase.h"

namespace
{
    //CSVファイル名
	const wchar_t* const worm_csv_name = L"Stage1/WormEnemy";
	//CSVでの胴体の数の要素番号
	constexpr int segment_count_number = 10;
	//CSVでの移動方向の要素番号
	constexpr int direction_number = 11;
	//CSVでのプレイヤーのZ座標がどれぐらいに来たら動き出すかの要素番号
	constexpr int active_player_z_number = 12;
}

std::vector<std::shared_ptr<EnemyBase>> WormEnemyDataSetter::CreateEnemy(
    std::weak_ptr<Player> pPlayer,
     std::weak_ptr<CameraBase> pCamera,
      std::weak_ptr<BulletManager> pBulletManager,
      std::weak_ptr<SoundManager> pSoundManager,
      std::weak_ptr<EffectManager> pEffectManager)
{
    //最終的な返り値
    std::vector<std::shared_ptr<EnemyBase>> pEnemies;

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
        int segmentNum = std::stoi(dataString[segment_count_number]);

        //移動方向(Z+:1,Z-:-1)
        float direction = std::stof(dataString[direction_number]);

        //プレイヤーがどの位置(Z)まで来たら動き出すか
        float activatePlayerZ = std::stof(dataString[active_player_z_number]);

        WormEnemy::WormEnemyData wormData;
        wormData.modelID = modelID;
        wormData.pos = pos;
        wormData.segmentCount = segmentNum;
        wormData.direction = direction;
        wormData.activatePlayerZ = activatePlayerZ;

        //その位置と、pCameraで敵を一つ作成する
        pEnemies.push_back(std::make_shared<WormEnemy>(
            pPlayer,//プレイヤー
            pBulletManager,//バレットマネージャー
            pCamera,//カメラ
            wormData,
            pSoundManager,
            pEffectManager
        ));
    }

    return pEnemies;
}
