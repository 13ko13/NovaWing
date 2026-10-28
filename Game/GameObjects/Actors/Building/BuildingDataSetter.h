#pragma once
#include <memory>
#include <vector>
#include <string>

class Building;
class CameraBase;
class BuildingDataSetter
{
public:
    BuildingDataSetter();
	virtual ~BuildingDataSetter();

	//ビルを生成する
	static std::vector<std::shared_ptr<Building>> CreateBuilding(
		std::weak_ptr<CameraBase> pCamera,
		float offsetZ,
		const std::wstring& csvFolderName
	);
};
