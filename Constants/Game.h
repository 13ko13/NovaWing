#pragma once
#include "Utility/Vector3.h"

namespace Game
{
	//ゲーム形を作る際に必要な定数
#ifdef _DEBUG
	//デバッグ中はウィンドウモードにする
	constexpr int screen_width = 1280;
	constexpr int screen_height = 720;
#else
	constexpr int screen_width = 1920;
	constexpr int screen_height = 1080;
#endif

	//UIのスケール計算の基準となる解像度
	//screen_width/heightがDebug/Releaseで異なっていても、
	//UIの見た目の大きさをこの解像度基準で揃えるために使う
	constexpr int base_screen_width = 1920;
	constexpr int base_screen_height = 1080;

	constexpr int color_bit_num = 32;

	//海面からどれぐらい離れたところにプレイヤーを押し戻すか
	constexpr float sea_player_margin = 200.0f;
	//海面からどれぐらい離れたところにカメラを押し戻すか
	constexpr float sea_camera_margin = 300.0f;

	//ライトの方向(光が進む向き)
	//夜空のスカイボックス(Data/Image/SkyBoxNight)の月の方向から差す光にしている
	//月は正面(+Z)の面の(720,215)にあるので、その方向(0.332,0.473,0.816)の逆向き
	//月の位置を変えたら、ここも合わせること
	const Vector3 light_direction = Vector3(-0.332f, -0.473f, -0.816f);
}