#pragma once
#include <map>
#include <vector>
#include <string>

#include "../Utility/Vector2.h"

//入力イベント名(InputManagerの対応表・IsTriggered等の呼び出しで共通して使う定数)
//注意:ヘッダに書かないと別のクラスから使用することができない
namespace InputEvent
{
	constexpr const char* ok = "ok";
	constexpr const char* next = "next";
	constexpr const char* shoot = "shoot";
	constexpr const char* somersault = "somersault";
	constexpr const char* boost = "boost";
	constexpr const char* brake = "brake";
	constexpr const char* up = "up";
	constexpr const char* down = "down";
	constexpr const char* any_key = "anyKey";

#ifdef _DEBUG
	//デバッグ用
	constexpr const char* restart = "restart";
	constexpr const char* gaugeUp = "gaugeUp";
	constexpr const char* gaugeDown = "gaugeDown";
	constexpr const char* bossWarp = "bossWarp";
	constexpr const char* killBoss = "killBoss";
	constexpr const char* downScanlineFrequency = "downScanlineFrequency";
	constexpr const char* upScanlineFrequency = "upScanlineFrequency";
#endif
}

/// <summary>
/// 周辺機器種別
/// </summary>
enum class PeripheralType
{
	keyboard,
	pad1
};

/// <summary>
/// 入力対応情報
/// </summary>
struct InputState
{
	PeripheralType type;//入力された機器の種別
	int id;//入力情報が入る(キーボードの場合はインデックス、パッドの場合はビット)
};

/// <summary>
/// 入力を抽象化するためのシングルトンクラス
/// </summary>
class InputManager
{
	//mapは対応表のようなもの
private:
	std::map<std::string, std::vector<InputState>> m_inputTable;///イベント名と実際の入力の対応表
	std::map<std::string, bool>m_inputData;///実際に入力されたかどうかのデータ
	std::map<std::string, bool>m_lastInputData;///前のフレームに入力されたかどうかのデータ

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
	/// <param name="name">イベント名(例:OK,BACKなど)</param>
	/// <returns>押されている:true,押されていない:false</returns>
	bool IsPressed(const char* name) const;

	/// <summary>
	/// 特定のボタンが現在押されたか(押された瞬間のみ反応する)
	/// </summary>
	/// <param name="name">イベント名</param>
	/// <returns>今押された瞬間:true,押されてないor押しっぱなし:false</returns>
	bool IsTriggered(const char* name) const;

	/// <summary>
	/// 特定のボタンが離された瞬間のみ反応する
	/// </summary>
	/// <param name="name">イベント名</param>
	/// <returns>離された瞬間:true,まだ押されていたらfalse</returns>
	bool IsReleased(const char* name) const;

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