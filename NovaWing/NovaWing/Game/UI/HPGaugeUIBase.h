#pragma once
#include <memory>

#include "UIBase.h"

class Charactor;
class Vector2;
class HPGaugeUIBase : public UIBase
{
public:
	HPGaugeUIBase(std::weak_ptr<Charactor> pCharactor);
	virtual ~HPGaugeUIBase();

	void Update() override;

private:

protected:
	//時間
	int m_frame = 0;

	//UIにかけるグリッチシェーダのハンドル
	int m_glitchPSH = -1;

	//グリッチシェーダに渡すためのシェーダバッファ
	struct GlitchBuffer
	{
		float time;
		float scanlineFrequency;//スキャンラインを入れる周期(数字が大きいほど周期が短い)
		float dummy[2];//16バイトアライメント
	};
	int m_cbufferGlitch = -1;
	GlitchBuffer* m_pCBuffGlitchData = nullptr;

	//HP参照対象のキャラ
	std::weak_ptr<Charactor> m_pHpTargetCharactor;

protected:
	void DrawHPGauge(
		int frameHandle,
		int gaugeHandle,
		const Vector2& drawPos,
		bool isBoss = false
	);
};