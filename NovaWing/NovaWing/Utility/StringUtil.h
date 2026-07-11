#pragma once
#include <DxLib.h>
#include <string>

namespace StringUtil
{
	/// <summary>
	/// char->TCHARの変換
	/// </summary>
	/// <param name="character"></param>
	/// <returns></returns>
	TCHAR* ToTCHAR(const char* character);

	/// <summary>
	/// 文字数がある数を超えるたびに改行を挟み、その文字列を返す関数
	/// </summary>
	/// <param name="str">対象の文字列</param>
	/// <param name="maxLength">1行あたりの最大文字数</param>
	/// <returns>改行を挿入した文字列</returns>
	std::wstring InsertNewLines(const std::wstring& str, int maxLength);

	/// <summary>
	/// 行の数を数える関数
	/// </summary>
	/// <param name="str">対象の文字列</param>
	/// <returns>行の数</returns>
	int WStringLineNum(const std::wstring& str);

	/// <summary>
	///  wstringをstringに変換する関数
	/// </summary>
	/// <param name="wstr">対象のwstring</param>
	/// <returns>変換後のstring</returns>
	std::string WstringToString(const std::wstring& wstr);

	/// <summary>
	/// stringをwstringに変換する関数
	/// </summary>
	/// <param name="str">対象のstring</param>
	/// <returns>変換後のwstring</returns>
	std::wstring StringToWstring(const std::string& str);

	/// <summary>
	/// 文字列の幅を取得する関数
	/// </summary>
	/// <param name="str">対象の文字列</param>
	/// <param name="fontHandle">フォントハンドル</param>
	/// <returns>文字列の幅</returns>
	float GetTextWidth(const std::wstring& str, int fontHandle);
}
