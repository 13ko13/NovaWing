#pragma once

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
	//水滴用のピクセルシェーダーハンドル
	int m_waterDropEffectPSH = -1;
	//水滴エフェクトをかける用のレンダーターゲットハンドル
	int m_screenH = 0;

	// struct WaterDropBuffer
	// {
	// 	float padding[4];//16バイトアライメント
	// };
	// //水滴用の定数バッファハンドル
	// int m_cbuffWaterDropH = -1;
	// //水滴用の定数バッファ
	// WaterDropBuffer* m_pCBuffWaterDrop = nullptr;
};
