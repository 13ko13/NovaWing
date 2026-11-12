#include <DxLib.h>

#include "WaterDropEffect.h"
#include "Main/Application.h"
#include "Utility/GraphShaderDraw.h"
#include "Constants/ShaderRegister.h"

WaterDropEffect::WaterDropEffect()
{
	//水滴用のシェーダーをロード
	m_waterDropEffectPSH = LoadPixelShader(L"WaterDropEffectPS.pso");

	//画面サイズ
	const auto& wsize = Application::GetInstance().GetWindowSize();

	//UIを除いた画面全体にブーストエフェクトを描画するための
	//オフスクリーンを作成
	m_screenH = MakeScreen(wsize.width, wsize.height);

	//水滴用の定数バッファを作成
	m_cbuffWaterDropH = CreateShaderConstantBuffer(sizeof(WaterDropBuffer));
	//定数バッファの内容を取得
	m_pCBuffWaterDrop = static_cast<WaterDropBuffer*>(GetBufferShaderConstantBuffer(m_cbuffWaterDropH));

	m_pCBuffWaterDrop->waterData[0].radius = 0.03f;
	m_pCBuffWaterDrop->waterData[0].pos = Vector2(0.8f, 0.2f);
	m_pCBuffWaterDrop->waterData[1].radius = 0.05f;
	m_pCBuffWaterDrop->waterData[1].pos = Vector2(0.2f, 0.8f);
	m_pCBuffWaterDrop->waterData[2].radius =0.07f;
	m_pCBuffWaterDrop->waterData[2].pos = Vector2(0.5f, 0.8f);
	for(int i = 3; i < max_water_drop_count; i++)
	{
		m_pCBuffWaterDrop->waterData[i].radius = 0.0f;
		m_pCBuffWaterDrop->waterData[i].pos = Vector2(0.0f, 0.0f);
	}
}

WaterDropEffect::~WaterDropEffect()
{
	DeleteShaderConstantBuffer(m_cbuffWaterDropH);
}

void WaterDropEffect::Update()
{
}

void WaterDropEffect::Draw()
{
	//水滴のピクセルシェーダーを有効にする
	SetUsePixelShader(m_waterDropEffectPSH);
	//定数バッファの更新
	UpdateShaderConstantBuffer(m_cbuffWaterDropH);
	//定数バッファをセットする
	SetShaderConstantBuffer(m_cbuffWaterDropH, DX_SHADERTYPE_PIXEL, ShaderRegister::cbuffer_water_drop);

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
	//定数バッファをリセット
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, ShaderRegister::cbuffer_water_drop);
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
