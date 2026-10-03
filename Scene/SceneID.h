#pragma once

enum class SceneID : int
{
	None,//シーンが何もない状態(起動直後など)
	Title,//タイトル
	Game,//ゲーム
	Pause,//ポーズ
	Gameover,//ゲームオーバー
	Clear,//クリア
};
