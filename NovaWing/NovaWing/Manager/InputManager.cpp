#include "InputManager.h"
#include <DxLib.h>

namespace
{
	constexpr int min_dedzone_r = 3000;//右スティックの最小デッドゾーン
	constexpr int max_dedzone_r = 28000;//右スティックの最大デッドゾーン
}

InputManager::InputManager() :
	m_inputData{},
	m_lastInputData{},
	m_inputTable{},
	m_bufX(0),
	m_bufY(0),
	m_rightStickDir({ 0.0f,0.0f })
{
	//イベント名を添え時にして、右辺値に実際の入力種別と押されたボタンの配列を置く
	m_inputTable["ok"] = { {PeripheralType::keyboard,KEY_INPUT_A},	//キーボード:エンターキー
							{PeripheralType::pad1,PAD_INPUT_A } };	//パッド:A

	m_inputTable["shoot"] = { {PeripheralType::keyboard,KEY_INPUT_Z},	//キーボード:Z
							{PeripheralType::pad1,PAD_INPUT_B} };		//パッド:Bボタン

	m_inputTable["somersault"] = { {PeripheralType::keyboard,KEY_INPUT_Z},	//キーボード:Z
							{PeripheralType::pad1,PAD_INPUT_X} };//パッド:Yボタン

	m_inputTable["boost"] = { {PeripheralType::keyboard,KEY_INPUT_Z},	//キーボード:Z
							{PeripheralType::pad1,PAD_INPUT_C} };//パッド:?ボタン

	m_inputTable["brake"] = { {PeripheralType::keyboard,KEY_INPUT_Z},	//キーボード:Z
							{PeripheralType::pad1,PAD_INPUT_A} };//パッド:Aボタン

	m_inputTable["up"] = { {PeripheralType::keyboard,KEY_INPUT_UP},		//キーボード:上矢印
						{PeripheralType::pad1,PAD_INPUT_UP } };		//パッド:スティック上又は十字上

	m_inputTable["down"] = { {PeripheralType::keyboard,KEY_INPUT_DOWN},	//キーボード:下矢印
							{PeripheralType::pad1,PAD_INPUT_DOWN } };	//パッド:スティック下又は十字下
	
#ifdef _DEBUG
	//デバッグ用キー
	m_inputTable["left"] = { {PeripheralType::keyboard,KEY_INPUT_LEFT},	//キーボード:左矢印
								{PeripheralType::pad1,PAD_INPUT_LEFT } };//パッド:スティック左又は十字左

	m_inputTable["right"] = { {PeripheralType::keyboard,KEY_INPUT_RIGHT},//キーボード:右矢印
							{PeripheralType::pad1,PAD_INPUT_RIGHT } };	 //パッド:スティック右又は十字右
#endif

	//あらかじめ入力データのための枠を開けておく
	//ここで枠を開けておかないと、チェックの際にAt関数でクラッシュする可能性がある(Null)
	for (const auto& inputInfo : m_inputTable)//forのinputInfoが変更されないようにconstにする
	{
		m_inputData[inputInfo.first] = false;
		m_lastInputData[inputInfo.first] = false;
	}

	//AnyKey,AnyButtonの枠も開けておく
	m_inputData["anyKey"] = false;
	m_lastInputData["anyKey"] = false;
}

void InputManager::Update()
{
	//まず入力情報を取得
	char keyState[256];
	GetHitKeyStateAll(keyState);//生のキーボード情報
	int padState = GetJoypadInputState(DX_INPUT_PAD1);//生のPAD1情報
	m_lastInputData = m_inputData;//直前の入力を保存する(前のフレーム情報をコピー)
	//すべての入力イベントをチェックします
	//ここでinputDataが更新される
	//inputTableを回して各イベントの入力をチェックする
	for (const auto& inputInfo : m_inputTable)
	{
		auto& input = m_inputData[inputInfo.first];//inputInfo.firstには"ok"等が入っている
		//inputを書き換えると、inputDataのそのイベントが押されてるかどうかを
		//書き換えることになる
		for (const auto& state : inputInfo.second)//InputStateのベクタを回す
		{
			//このループはInputState配列のループなので
			//まず、入力種別をチェックする
			switch (state.type)
			{
			case PeripheralType::keyboard://キーボードの場合
				//GetHitKeyStateAllでとってきた配列の中身を見て
				//入力されているかどうかをチェック
				input = keyState[state.id];
				break;
			case PeripheralType::pad1:
				//GetJoypadInputStateでとってきたビット情報を見て
				//登録されているビット情報と&を取り、そのビットが立っているかどうかを見る
				input = (padState & state.id);
				break;
			}
			if (input)	//大事
			{
				//breakしないとpad1がそのまま残ってしまう
				//最後のチェックで押されてないとfalseになってしまう
				break;
			}
		}
	}

	//アナログスティック(左)の値を取得し続ける
	GetJoypadAnalogInput(&m_bufX, &m_bufY, DX_INPUT_PAD1);
	//アナログスティック(右)の値を取得し続ける
	XINPUT_STATE input;
	GetJoypadXInputState(DX_INPUT_PAD1, &input);

	//右スティックの値をfloatに変換する
	Vector2 result = {
		static_cast<float>(input.ThumbRX),
		static_cast<float>(input.ThumbRY) };

	//入力ベクトルの長さを求める
	float len = result.Length();

	//min_dedzone_rが0.0,max_dedzone_rが1.0になるように計算
	//3000以下はマイナス、28000以上は1.0を超えている
	float lenRate = (len - min_dedzone_r) / (max_dedzone_r - min_dedzone_r);
	if (lenRate < 0.0f) lenRate = 0.0f;
	if (lenRate > 1.0f) lenRate = 1.0f;

	m_rightStickDir = result.Normalized() * lenRate;

	//----------AnyKey,AnyButtonの処理-----------
	bool anyPressed = false;

	//キーボード
	//keyStateをループで全部見てどれか一つでも押されていたらtrueにする
	for (int i = 0; i < 256; i++)
	{
		if (keyState[i])
		{
			anyPressed = true;
			break;
		}
	}

	//パッド
	//padStateのビットが0以外ならどれかのボタンが押されていることになる
	if (padState != 0)
	{
		anyPressed = true;
	}

	//結果を保存する
	m_inputData["anyKey"] = anyPressed;
}

InputManager& InputManager::GetInstance()
{
	//staticでインスタンスを宣言してそれを返す
	static InputManager instance;
	return instance;
}

bool InputManager::IsPressed(const char* name)const
{
	//もし「ない」イベントを送られるとクラッシュします
	//もしクラッシュがいやだったら
	//if(inputData.contains(name))
	//{
	//	 return false;//で回避できます。
	//}
	return m_inputData.at(name);//const関数内部なので[]ではなくatを使用してる
}

bool InputManager::IsTriggered(const char* name)const
{
	// 今入力されているボタンと
	// 前のフレームで入力されているボタンを比較する
	return m_inputData.at(name) && !m_lastInputData.at(name);
}

bool InputManager::IsReleased(const char* name) const
{
	//前のフレームで入力されているボタンと
	//現在入力されているかどうかを比較して
	//入力されていなかったらtrueにする
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




