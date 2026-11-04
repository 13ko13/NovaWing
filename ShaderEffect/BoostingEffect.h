#pragma once

class BoostingEffect
{
public:
    BoostingEffect();
	virtual ~BoostingEffect();
	
	/// 更新
	void Update(bool isBoosting);

	//描画先をブーストエフェクト用のレンダーターゲットに変更
	void ChangeBoostEffectRT();
	//描画先をもとに戻す
	void ChangeNormalRT();

private:
	//ブースト用のピクセルシェーダーハンドル
	int m_boostingEffectPSH = -1;
	//ブーストエフェクトをかける用のレンダーターゲットハンドル
	int m_screenH = 0;
};
