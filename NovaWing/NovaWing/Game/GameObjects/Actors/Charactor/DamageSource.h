#pragma once

//ダメージの発生源の種類
enum class DamageSourceType
{
	Rock,//岩
	Worm,//ワームエネミー
	Beam,//ボスのビーム
};

//ダメージの発生源(種類+個体ID)
struct DamageSource
{
	DamageSourceType type;
	int id;

	//std::setで扱うための比較演算子
	//std::setは要素が重複しないコンテナ
	//自分のIDと相手のIDを比較して小さい方を返す
	//同じダメージソースであればこの順番に並び替えてください
	//というような、基準を決めるための演算子
	bool operator<(const DamageSource& other) const
	{
		if (type != other.type)
		{
			return type < other.type;
		}
		return id < other.id;
	}
};