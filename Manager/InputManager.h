#pragma once
#include <map>
#include <vector>
#include <string>

#include "../Utility/Vector2.h"

//入力イベント(InputManagerの対応表・IsTriggered等の呼び出しで共通して使う)
//注意:ヘッダに書かないと別のクラスから使用することができない
enum class InputEvent
{
	ok,
	next,
	shoot,
	somersault,
	boost,
	brake,
	up,
	down,
	any_key,
	pause,
	close,
	right_rolling,
	left_rolling,

#ifdef _DEBUG
	//デバッグ用
	gaugeUp,
	gaugeDown,
	killBoss,
	downScanlineFrequency,
	upScanlineFrequency,
	Tutorial
#endif
};

/// <summary>
/// 周辺機器種別
/// </summary>
enum class PeripheralType
{
	keyboard,
	pad1,//XInput対応ゲームパッドのボタン
	pad1_lstick_up,//左スティックを上に倒した(idは未使用)
	pad1_lstick_down//左スティックを下に倒した(idは未使用)
};

/// <summary>
/// 入力対応情報
/// </summary>
struct InputState
{
	PeripheralType type;//入力された機器の種別
	int id;//キーボード:キーコード / パッド:XINPUT_BUTTON_*(Buttons配列のインデックス) / 左スティック:未使用
};

/// <summary>
/// 入力を抽象化するためのシングルトンクラス
/// </summary>
class InputManager
{
	//mapは対応表のようなもの
private:
	std::map<InputEvent, std::vector<InputState>> m_inputTable;///イベントと実際の入力の対応表
	std::map<InputEvent, bool>m_inputData;///実際に入力されたかどうかのデータ
	std::map<InputEvent, bool>m_lastInputData;///前のフレームに入力されたかどうかのデータ

	//コントローラーの左スティックを倒したときの値を保持するもの
	int m_bufX;
	int m_bufY;

	//右スティックの入力方向(正規化済)と入力強度を保持する
	Vector2 m_rightStickDir;
private:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	InputManager();

public:
	/// <summary>
	/// InputManagerのインスタンスを取得する
	/// </summary>
	/// <returns></returns>
	static InputManager& GetInstance();

	//コピーと代入を禁止する(消す)
	InputManager(const InputManager&) = delete;
	InputManager& operator=(const InputManager&) = delete;

	/// <summary>
	/// 毎フレーム呼び出して、
	/// 入力情報を更新させる
	/// </summary>
	void Update();

	/// <summary>
	/// 特定のボタンがおされているか
	/// </summary>
	/// <param name="event">入力イベント(例:ok,closeなど)</param>
	/// <returns>押されている:true,押されていない:false</returns>
	bool IsPressed(InputEvent event) const;

	/// <summary>
	/// 特定のボタンが現在押されたか(押された瞬間のみ反応する)
	/// </summary>
	/// <param name="event">入力イベント</param>
	/// <returns>今押された瞬間:true,押されてないor押しっぱなし:false</returns>
	bool IsTriggered(InputEvent event) const;

	/// <summary>
	/// 特定のボタンが離された瞬間のみ反応する
	/// </summary>
	/// <param name="event">入力イベント</param>
	/// <returns>離された瞬間:true,まだ押されていたらfalse</returns>
	bool IsReleased(InputEvent event) const;

	/// <summary>
	/// コントローラーの左スティックを倒したときどのくらい倒したかのXの値を取得する
	/// </summary>
	/// <returns>左スティックを倒したときどのくらい倒したかの値</returns>
	int GetBufX();

	/// <summary>
	/// コントローラーの左スティックを倒したときどのくらい倒したかのYの値を取得する
	/// </summary>
	/// <returns>左スティックを倒したときどのくらい倒したかの値</returns>
	int GetBufY();

	/// <summary>
	/// 右スティックの倒している入力方向(正規化済み)と
	/// 入力強度を取得する
	/// </summary>
	/// <returns>右スティックの倒している入力方向(正規化済み)と入力強度</returns>
	Vector2 const GetRightStickDir() const {return m_rightStickDir;}
};