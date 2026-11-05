#pragma once

class BoostingEffect
{
public:
    BoostingEffect();
	virtual ~BoostingEffect();
	
	/// 更新
	void Draw(bool isBoosting);

	//描画先をブーストエフェクト用のレンダーターゲットに変更
	void ChangeBoostEffectRT();
	//描画先をもとに戻す
	void ChangeNormalRT();

private:
	//ブースト用のピクセルシェーダーハンドル
	int m_boostingEffectPSH = -1;
	//ブーストエフェクトをかける用のレンダーターゲットハンドル
	int m_screenH = 0;

	struct BoostingBuffer
	{
		float boostAmount;//ブースト量
		float padding[3];//16バイトアライメント
	};
	//ブースト用の定数バッファハンドル
	int m_cbuffBoostingH = -1;
	//ブースト用の定数バッファ
	BoostingBuffer* m_pCBuffBoosting = nullptr;

	//ブースト量
	float m_boostAmount = 0.0f;
};
