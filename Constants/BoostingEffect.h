#pragma once

class BoostingEffect
{
public:
    BoostingEffect();
    virtual ~BoostingEffect();
    /// ブースト開始
    void StartBoost();
    /// ブースト終了
    void EndBoost();
    /// 描画
    void Draw();
    /// 更新
    void Update(bool isBoosting);

private:
	//ブースト用のピクセルシェーダーハンドル
	int m_boostingEffectPSH = -1;
	//ブーストエフェクトをかける用のレンダーターゲットハンドル
	int m_screenH = 0;
};
