#pragma once
#include <memory>
#include <vector>
#include <string>

#include "Manager/ResourceLoader.h"

class CameraBase;
class Rock;
class RockDataSetter
{
public:
    //岩を作成して作成した岩を配列に入れて返す
    static std::vector<std::shared_ptr<Rock>> CreateRock(
        std::weak_ptr<CameraBase> pCamera);

private:
    //wstringをModelIDに変換する
    static ResourceLoader::ModelID WStringToModelID(const std::wstring id);
};