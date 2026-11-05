#include <DxLib.h>
#include <algorithm>

#include "BoostingEffect.h"
#include "Main/Application.h"
#include "Utility/GraphShaderDraw.h"
#include "Constants/ShaderRegister.h"

namespace
{
	//ブースト量を毎フレーム増やす量
	constexpr float boost_amount_per_frame = 0.05f;
}

BoostingEffect::BoostingEffect()
{
	//ブースト用のシェーダーをロード
	m_boostingEffectPSH = LoadPixelShader(L"BoostingEffectPS.pso");

	//画面サイズ
	const auto& wsize = Application::GetInstance().GetWindowSize();

	//UIを除いた画面全体にブーストエフェクトを描画するための
	//オフスクリーンを作成
	m_screenH = MakeScreen(wsize.width, wsize.height);

	//ブースト用の定数バッファを作成
	m_cbuffBoostingH = CreateShaderConstantBuffer(sizeof(BoostingBuffer));
	//定数バッファの内容を取得
	m_pCBuffBoosting = static_cast<BoostingBuffer*>(GetBufferShaderConstantBuffer(m_cbuffBoostingH));
}

BoostingEffect::~BoostingEffect()
{
	DeleteShaderConstantBuffer(m_cbuffBoostingH);
}

void BoostingEffect::Draw(bool isBoosting)
{
	//画面サイズ
	const auto& wsize = Application::GetInstance().GetWindowSize();

	//ブースト中であればブースト量を増やす
	if (isBoosting) m_boostAmount += boost_amount_per_frame;
	else m_boostAmount -= boost_amount_per_frame;
	//ブースト量を定数バッファに書き込む
	m_pCBuffBoosting->boostAmount = m_boostAmount;
	//定数バッファの内容を更新
	UpdateShaderConstantBuffer(m_cbuffBoostingH);

	//ブースト量にちょっとでも正の値が入っていればブーストエフェクトを描画する
	if (m_boostAmount > 0.0f)
	{
		//定数バッファをピクセルシェーダーにセット
		SetShaderConstantBuffer(m_cbuffBoostingH, DX_SHADERTYPE_PIXEL, ShaderRegister::cbuffer_boosting);

		//シェーダーを通して描画する
		SetUsePixelShader(m_boostingEffectPSH);
		DrawGraphToShaderByCenter(
			static_cast<float>(wsize.width / 2),
			static_cast<float>(wsize.height / 2),
			1.0, m_screenH);

		//シェーダーを開放
		SetUsePixelShader(-1);
		//定数バッファをピクセルシェーダーから外す
		SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, ShaderRegister::cbuffer_boosting);
	}
	else
	{
		//定数バッファをピクセルシェーダーにセット
		SetShaderConstantBuffer(m_cbuffBoostingH, DX_SHADERTYPE_PIXEL, ShaderRegister::cbuffer_boosting);
		//ブーストしていなければシェーダーを通さずに描画
		DrawGraph(0, 0, m_screenH, TRUE);
		//シェーダーを開放
		SetUsePixelShader(-1);
		//定数バッファをピクセルシェーダーから外す
		SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, ShaderRegister::cbuffer_boosting);
	}
	//ブースト量を0.0～1.0の範囲に収める
	m_boostAmount = std::clamp(m_boostAmount, 0.0f, 1.0f);
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
