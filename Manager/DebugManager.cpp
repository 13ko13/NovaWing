#include "DebugManager.h"

DebugManager::DebugManager()
{
}

DebugManager& DebugManager::GetInstance()
{
	// staticでインスタンスを宣言してそれを返す
	static DebugManager instance;
	return instance;
}
