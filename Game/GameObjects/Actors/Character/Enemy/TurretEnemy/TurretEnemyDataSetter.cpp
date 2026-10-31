#include "TurretEnemyDataSetter.h"
#include "Manager/ResourceLoader.h"
#include "Manager/CSVDataLoader.h"
#include "CSVData/CSVData.h"
#include "Utility/Vector3.h"
#include "Game/GameObjects/Actors/Character/Enemy/EnemyBase.h"
#include "TurretEnemy.h"

namespace
{
	//CSVファイル名
	const wchar_t* const turret_enemy_csv_name = L"TurretEnemy";
}

std::vector<std::shared_ptr<EnemyBase>> TurretEnemyDataSetter::CreateEnemy(
	std::weak_ptr<Player> pPlayer,
	std::weak_ptr<CameraBase> pCamera,
	std::weak_ptr<BulletManager> pBulletManager,
	std::weak_ptr<SoundManager> pSoundManager,
	std::weak_ptr<EffectManager> pEffectManager,
	float offsetZ, const std::wstring& csvFolderName)
{
	//最終的な返り値
	std::vector<std::shared_ptr<EnemyBase>> pEnemies;

	//CSVLoaderにCSVをロードさせる
	CSVDataLoader& loader = CSVDataLoader::GetInstance();

	//CSVデータのリストを受け取る
	std::wstring csvPath = csvFolderName + L"/" + turret_enemy_csv_name;//csvフォルダの名前/csvファイル名
	std::vector<std::shared_ptr<CSVData>> pData = loader.LoadCSV(csvPath.c_str());

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

		pos.z += offsetZ;

		TurretEnemy::TurretEnemyData turretData;
		turretData.modelID = modelID;
		turretData.pBulletManager = pBulletManager;
		turretData.pCamera = pCamera;
		turretData.pEffectManager = pEffectManager;
		turretData.pPlayer = pPlayer;
		turretData.pos = pos;
		turretData.pSoundManager = pSoundManager;

		//その位置と、pCameraで敵を一つ作成する
		pEnemies.push_back(std::make_shared<TurretEnemy>(turretData));
	}

	return pEnemies;
}
