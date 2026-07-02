#include "Application.h"
#include <DxLib.h>

// プログラムは WinMain から始まる
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	auto& app = Application::GetInstance();
	if (!app.Init())
	{
		return -1;
	}
	app.Run();
	app.Terminate();
	return 0;
}