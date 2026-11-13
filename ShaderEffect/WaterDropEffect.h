#pragma once
#include "Utility/Vector2.h"

class WaterDropEffect
{
public:
	WaterDropEffect();
	~WaterDropEffect();

	//更新
	void Update();
	//描画
	void Draw();

	//描画先を水滴用のレンダーターゲットに変更
	void ChangeWaterDropEffectRT();
	//描画先をもとに戻す
	void ChangeNormalRT();

private:
	//0.0f~1.0fの乱数を返す
	float GetRandRatio();
	//水滴を生み出す
	//index番目の水滴に位置と半径を入れます。
	void RespawnWaterDrop(int index);

private:
	//水滴用のピクセルシェーダーハンドル
	int m_waterDropEffectPSH = -1;
	//水滴エフェクトをかける用のレンダーターゲットハンドル
	int m_screenH = 0;

	struct WaterDropData
	{
		float radius;//半径
		Vector2 pos;//位置
		float padding;//16バイトアライメント
	};

	static const int max_water_drop_count = 15;

	struct WaterDropBuffer
	{
		WaterDropData waterData[max_water_drop_count];
	};
	//水滴用の定数バッファハンドル
	int m_cbuffWaterDropH = -1;
	//水滴用の定数バッファ
	WaterDropBuffer* m_pCBuffWaterDrop = nullptr;
};
