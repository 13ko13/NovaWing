#include <DxLib.h>

#include "BoostingEffect.h"
#include "Main/Application.h"

BoostingEffect::BoostingEffect()
{
	//ブースト用のシェーダーをロード
	m_boostingEffectPSH = LoadPixelShader(L"BoostingEffectPS.pso");
}

BoostingEffect::~BoostingEffect()
{
}

void BoostingEffect::StartBoost()
{
	//ウィンドウサイズを取得
	const auto& wsize = Application::GetInstance().GetWindowSize();

	//ブースト用のレンダーターゲットを生成
	m_screenH = MakeScreen(
		wsize.width,
		wsize.height
	);
}

void BoostingEffect::EndBoost()
{
}

void BoostingEffect::Draw()
{
}

void BoostingEffect::Update(bool isBoosting)
{
	
}
