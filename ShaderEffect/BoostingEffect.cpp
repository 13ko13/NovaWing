#include <DxLib.h>

#include "BoostingEffect.h"
#include "Main/Application.h"

BoostingEffect::BoostingEffect()
{
	// //ブースト用のシェーダーをロード
	// m_boostingEffectPSH = LoadPixelShader(L"BoostingEffectPS.pso");

	//画面サイズ
	const auto& wsize = Application::GetInstance().GetWindowSize();

	//UIを除いた画面全体にブーストエフェクトを描画するための
	//オフスクリーンを作成
	m_screenH = MakeScreen(wsize.width, wsize.height);
}

BoostingEffect::~BoostingEffect()
{
}

void BoostingEffect::Update(bool isBoosting)
{
	//ブースト中ならオフスクリーンを描画
	if(isBoosting)
	{
		//画面サイズ
		const auto& wsize = Application::GetInstance().GetWindowSize();
		DrawGraph(wsize.width, wsize.height,m_screenH,true);
	}
}

void BoostingEffect::ChangeBoostEffectRT()
{
	//スクリーンをオフスクリーンに変更
	SetDrawScreen(m_screenH);

	//前フレームの描画内容をクリア
	ClearDrawScreen();
}

void BoostingEffect::ChangeNormalRT()
{
	//描画先をバックバッファに戻す
	SetDrawScreen(DX_SCREEN_BACK);
}
