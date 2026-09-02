#include "InputManager.h"
#include <DxLib.h>
#include <cmath>

namespace
{
	constexpr int min_dedzone_r = 3000;	 // 右スティックの最小デッドゾーン
	constexpr int max_dedzone_r = 28000; // 右スティックの最大デッドゾーン

	// 左スティック(移動用)の最小デッドゾーン
	constexpr int left_stick_deadzone = 8000;
	// XInputのThumbLX/LY(±32768)を従来のGetBufX/Yのスケール(±1000)に合わせる係数
	constexpr float left_stick_to_buf_scale = 1000.0f / 32768.0f;

	// XINPUT_STATE::Buttonsの要素数
	constexpr int xinput_button_num = 16;
} // namespace

InputManager::InputManager() :
	m_inputData{},
	m_lastInputData{},
	m_inputTable{},
	m_bufX(0),
	m_bufY(0),
	m_rightStickDir({ 0.0f, 0.0f })
{
	// イベント名を添え字にして、右辺値に実際の入力種別と押されたボタンの配列を置く
	// パッド側はXINPUT_BUTTON_*(GetJoypadXInputStateのButtons配列のインデックス)を指定する
	m_inputTable[InputEvent::ok] = { {PeripheralType::keyboard, KEY_INPUT_A}, // キーボード:A
									{PeripheralType::pad1, XINPUT_BUTTON_A} };	 // パッド:Aボタン

	m_inputTable[InputEvent::shoot] = { {PeripheralType::keyboard, KEY_INPUT_Z}, // キーボード:Z
									   {PeripheralType::pad1, XINPUT_BUTTON_B} };	// パッド:Bボタン

	m_inputTable[InputEvent::somersault] = { {PeripheralType::keyboard, KEY_INPUT_Z}, // キーボード:Z
											{PeripheralType::pad1, XINPUT_BUTTON_DPAD_UP} };	 // パッド:十字キー上

	m_inputTable[InputEvent::boost] = { {PeripheralType::keyboard, KEY_INPUT_Z}, // キーボード:Z
									   {PeripheralType::pad1, XINPUT_BUTTON_X} };	// パッド:Xボタン

	m_inputTable[InputEvent::brake] = { {PeripheralType::keyboard, KEY_INPUT_Z}, // キーボード:Z
									   {PeripheralType::pad1, XINPUT_BUTTON_A} };	// パッド:Aボタン

	m_inputTable[InputEvent::up] = { {PeripheralType::keyboard, KEY_INPUT_UP}, // キーボード:上矢印
									{PeripheralType::pad1_lstick_up, 0} };	  // パッド:左スティック上

	m_inputTable[InputEvent::down] = { {PeripheralType::keyboard, KEY_INPUT_DOWN}, // キーボード:下矢印
									  {PeripheralType::pad1_lstick_down, 0} };	  // パッド:左スティック下

	m_inputTable[InputEvent::next] = { {PeripheralType::keyboard, KEY_INPUT_A}, // キーボード:A
									  {PeripheralType::pad1, XINPUT_BUTTON_A} };	  // パッド:Aボタン

	m_inputTable[InputEvent::pause] = { {PeripheralType::pad1, XINPUT_BUTTON_START} };//STARTボタン

	m_inputTable[InputEvent::close] = { {PeripheralType::pad1, XINPUT_BUTTON_B } };//Bボタン

	m_inputTable[InputEvent::how_to] = { {PeripheralType::pad1, XINPUT_BUTTON_LEFT_SHOULDER } };//LBボタン

#ifdef _DEBUG
	m_inputTable[InputEvent::restart] = { {PeripheralType::pad1, XINPUT_BUTTON_START} };			// パッド:STARTボタン
	m_inputTable[InputEvent::gaugeUp] = { {PeripheralType::keyboard, KEY_INPUT_UP} };		// キーボード:上
	m_inputTable[InputEvent::gaugeDown] = { {PeripheralType::keyboard, KEY_INPUT_DOWN} }; // キーボード:下
	m_inputTable[InputEvent::bossWarp] = { {PeripheralType::keyboard, KEY_INPUT_W} }; // キーボード:W
	m_inputTable[InputEvent::killBoss] = { {PeripheralType::keyboard, KEY_INPUT_K} }; // キーボード:K
	m_inputTable[InputEvent::upScanlineFrequency] =
		{ {PeripheralType::keyboard, KEY_INPUT_Q} }; // キーボード:Q
	m_inputTable[InputEvent::downScanlineFrequency] =
		{ {PeripheralType::keyboard, KEY_INPUT_E} }; // キーボード:E
#endif

	// あらかじめ入力データのための枠を開けておく
	// ここで枠を開けておかないと、チェックの際にAt関数でクラッシュする可能性がある(Null)
	for (const auto& inputInfo : m_inputTable) // forのinputInfoが変更されないようにconstにする
	{
		m_inputData[inputInfo.first] = false;
		m_lastInputData[inputInfo.first] = false;
	}

	// AnyKey,AnyButtonの枠も開けておく
	m_inputData[InputEvent::any_key] = false;
	m_lastInputData[InputEvent::any_key] = false;
}

void InputManager::Update()
{
	// まず入力情報を取得
	char keyState[256];
	GetHitKeyStateAll(keyState);					   // 生のキーボード情報

	// XInputの状態をまとめて取得(ボタン・左右スティックすべてここから読む)
	XINPUT_STATE xinputState;
	GetJoypadXInputState(DX_INPUT_PAD1, &xinputState);

	m_lastInputData = m_inputData;					   // 直前の入力を保存する(前のフレーム情報をコピー)
	// すべての入力イベントをチェックします
	// ここでinputDataが更新される
	// inputTableを回して各イベントの入力をチェックする
	for (const auto& inputInfo : m_inputTable)
	{
		auto& input = m_inputData[inputInfo.first]; // inputInfo.firstには"ok"等が入っている
		// inputを書き換えると、inputDataのそのイベントが押されてるかどうかを
		// 書き換えることになる
		for (const auto& state : inputInfo.second) // InputStateのベクタを回す
		{
			// このループはInputState配列のループなので
			// まず、入力種別をチェックする
			switch (state.type)
			{
			case PeripheralType::keyboard: // キーボードの場合
				// GetHitKeyStateAllでとってきた配列の中身を見て
				// 入力されているかどうかをチェック
				input = keyState[state.id];
				break;
			case PeripheralType::pad1:
				// state.idにはXINPUT_BUTTON_*が入っている。
				// それをそのままButtons配列のインデックスとして使い、押されているか見る
				input = xinputState.Buttons[state.id];
				break;
			case PeripheralType::pad1_lstick_up:
				// 左スティックを上に倒しているか(ThumbLYは上方向が正)
				input = (xinputState.ThumbLY > left_stick_deadzone);
				break;
			case PeripheralType::pad1_lstick_down:
				// 左スティックを下に倒しているか
				input = (xinputState.ThumbLY < -left_stick_deadzone);
				break;
			}
			if (input) // 大事
			{
				// breakしないとpad1がそのまま残ってしまう
				// 最後のチェックで押されてないとfalseになってしまう
				break;
			}
		}
	}

	// 左スティック(移動用)をThumbLX/LYから取得する。
	// デッドゾーン処理をしたうえで、従来のGetBufX/Yのスケール(±1000)に変換する
	int rawLX = xinputState.ThumbLX;
	int rawLY = xinputState.ThumbLY;
	if (std::abs(rawLX) < left_stick_deadzone) rawLX = 0;
	if (std::abs(rawLY) < left_stick_deadzone) rawLY = 0;
	m_bufX = static_cast<int>(rawLX * left_stick_to_buf_scale);
	// XInputのThumbLYは上方向が正。従来のGetJoypadAnalogInputは下方向が正だったので符号を反転する
	m_bufY = static_cast<int>(-rawLY * left_stick_to_buf_scale);

	// 右スティックの値をfloatに変換する(上で取得済みのxinputStateを使う)
	Vector2 result = {
		static_cast<float>(xinputState.ThumbRX),
		static_cast<float>(xinputState.ThumbRY) };

	// 入力ベクトルの長さを求める
	float len = result.Length();

	// min_dedzone_rが0.0,max_dedzone_rが1.0になるように計算
	// 3000以下はマイナス、28000以上は1.0を超えている
	float lenRate = (len - min_dedzone_r) / (max_dedzone_r - min_dedzone_r);
	if (lenRate < 0.0f)
		lenRate = 0.0f;
	if (lenRate > 1.0f)
		lenRate = 1.0f;

	m_rightStickDir = result.Normalized() * lenRate;

	//AnyKey,AnyButtonの処理
	bool anyPressed = false;

	// キーボード
	// keyStateをループで全部見てどれか一つでも押されていたらtrueにする
	for (int i = 0; i < 256; i++)
	{
		if (keyState[i])
		{
			anyPressed = true;
			break;
		}
	}

	// パッド
	// XInputの全ボタンをループ、どれか一つでも押されていたらtrueにする
	for (int i = 0; i < xinput_button_num; i++)
	{
		if (xinputState.Buttons[i])
		{
			anyPressed = true;
			break;
		}
	}

	// 結果を保存する
	m_inputData[InputEvent::any_key] = anyPressed;
}

InputManager& InputManager::GetInstance()
{
	// staticでインスタンスを宣言してそれを返す
	static InputManager instance;
	return instance;
}

bool InputManager::IsPressed(const char* name) const
{
	// もし「ない」イベントを送られるとクラッシュします
	// もしクラッシュがいやだったら
	// if(inputData.contains(name))
	//{
	//	 return false;//で回避できます。
	// }
	return m_inputData.at(name); // const関数内部なので[]ではなくatを使用してる
}

bool InputManager::IsTriggered(const char* name) const
{
	// 今入力されているボタンと
	// 前のフレームで入力されているボタンを比較する
	return m_inputData.at(name) && !m_lastInputData.at(name);
}

bool InputManager::IsReleased(const char* name) const
{
	// 前のフレームで入力されているボタンと
	// 現在入力されているかどうかを比較して
	// 入力されていなかったらtrueにする
	return m_lastInputData.at(name) && !m_inputData.at(name);
}

int InputManager::GetBufX()
{
	return m_bufX;
}

int InputManager::GetBufY()
{
	return m_bufY;
}
