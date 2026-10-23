#pragma once
#include <memory>
#include <vector>
#include <string>

class CameraBase;
class Scenery;
class SceneryDataSetter
{
public:
	//CSVの内容から景色を作成して、作成した景色を配列に入れて返す
	static std::vector<std::shared_ptr<Scenery>> CreateScenery(
		std::weak_ptr<CameraBase> pCamera,
		const std::wstring& csvFolderName);
};
