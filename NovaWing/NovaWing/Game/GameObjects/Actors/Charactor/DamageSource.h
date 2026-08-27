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
	bool operator<(const DamageSource& other) const
	{
		if (type != other.type)
		{
			return type < other.type;
		}
		return id < other.id;
	}
};