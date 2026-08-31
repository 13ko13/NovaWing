#pragma once

namespace Game
{
	//ゲーム形を作る際に必要な定数
	constexpr int screen_width = 1920;
	constexpr int screen_height = 1080;
	constexpr int color_bit_num = 32;

	//海面からどれぐらい離れたところにプレイヤーを押し戻すか
	constexpr float sea_player_margin = 100.0f;
	//海面からどれぐらい離れたところにカメラを押し戻すか
	constexpr float sea_camera_margin = 300.0f;
}