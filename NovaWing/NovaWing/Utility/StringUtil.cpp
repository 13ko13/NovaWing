#include "StringUtil.h"
#include <cassert> 

TCHAR* StringUtil::ToTCHAR(const char* character)
{
	//char* → TCHAR* への変換
	TCHAR cT[256];
	//文字列をUTF-16文字列に変換する関数
	MultiByteToWideChar(CP_ACP, 0, character, -1, cT, 256);
	return cT;
}

std::wstring StringUtil::InsertNewLines(const std::wstring& str, int maxLength)
{
	std::wstring newStr;
	//あらかじめ必要なメモリを確保(メモリ最適化)
	newStr.reserve(str.size() + (str.size() / maxLength));
	//文字を数えていく
	size_t count = 0;
	for (auto character : str)
	{
		//文字を追加していく
		newStr += character;
		//改行が挟まった時カウントをリセットする
		if (character == '\n')//文字列ではないのでシングルクォーテーション
		{
			count = 0;
			continue;
		}
		//半角
		if (character < 0x80)//128
		{
			//カウント
			count++;
		}
		else
		{
			//カウント
			count += 2;
		}
		//1行の文字数がmaxLengthを超えたら改行を挟む
		if (count > maxLength)
		{
			//改行を挟む
			newStr += '\n';
			//カウントをリセットする
			count = 0;
		}
	}

	return newStr;
}

int StringUtil::WStringLineNum(const std::wstring& str)
{
	size_t count = 0;
	for (char c : str)
	{
		if(c == '\n')
		{
			count++;
		}
	}
	return static_cast<int>(count);
}

std::string StringUtil::WstringToString(const std::wstring& wstr)
{
	std::string ret;
	//1度目の呼び出しは文字列数を知るために行う
	auto result = WideCharToMultiByte(
		CP_ACP,
		0,
		wstr.c_str(),
		wstr.length(),
		nullptr, 
		0, 
		nullptr,
		nullptr);
	assert(result >= 0); //変換に失敗していないか確認
	ret.resize(result); //変換後の文字列数に合わせてサイズを変更
	//2度目の呼び出しで実際に変換を行う
	WideCharToMultiByte(
		CP_ACP,
		0,
		wstr.c_str(),
		wstr.length(),
		ret.data(),
		ret.size(),
		nullptr,
		nullptr);
	return ret;
}

float StringUtil::GetTextWidth(const std::wstring& str, int fontHandle)
{
	return static_cast<float>(GetDrawNStringWidthToHandle(
		str.c_str(),
		wcslen(str.c_str()),
		fontHandle));
}