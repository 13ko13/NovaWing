#include <DxLib.h>

#include "WaterDropEffect.h"
#include "Main/Application.h"
#include "Utility/GraphShaderDraw.h"

WaterDropEffect::WaterDropEffect()
{
	//水滴用のシェーダーをロード
	m_waterDropEffectPSH = LoadPixelShader(L"WaterDropEffectPS.pso");

	//画面サイズ
	const auto& wsize = Application::GetInstance().GetWindowSize();

	//UIを除いた画面全体にブーストエフェクトを描画するための
	//オフスクリーンを作成
	m_screenH = MakeScreen(wsize.width, wsize.height);
}

WaterDropEffect::~WaterDropEffect()
{
}

void WaterDropEffect::Update()
{
}

void WaterDropEffect::Draw()
{
	//水滴のピクセルシェーダーを有効にする
	SetUsePixelShader(m_waterDropEffectPSH);
	//画面サイズ
	const auto& wsize = Application::GetInstance().GetWindowSize();
	//シェーダーを通してオフスクリーンを描画
	DrawGraphToShaderByCenter(
		static_cast<float>(wsize.width / 2),
		static_cast<float>(wsize.height / 2),
		1.0, m_screenH
	);

	//水滴のピクセルシェーダーを無効にする
	SetUsePixelShader(-1);
}

void WaterDropEffect::ChangeWaterDropEffectRT()
{
	//水滴を描画する用のオフスクリーンに描画先を変更
	SetDrawScreen(m_screenH);
	//前フレームの絵をいったんクリア
	ClearDrawScreen();
}

void WaterDropEffect::ChangeNormalRT()
{
	//もとに戻す
	SetDrawScreen(DX_SCREEN_BACK);
}
