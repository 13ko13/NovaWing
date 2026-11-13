#include <DxLib.h>

#include "WaterDropEffect.h"
#include "Main/Application.h"
#include "Utility/GraphShaderDraw.h"
#include "Constants/ShaderRegister.h"

namespace
{
	//ランダムな小数を返すときの値ごとの細かさ
	constexpr int rand_resolution = 1000;

	//水滴の最低半径
	constexpr float min_radius = 0.005f;
	//水滴の最大半径
	constexpr float max_radius = 0.015f;

	//画面に現れる最大水滴数
	constexpr int max_water_drop_count = 10;

	//画面中央のuv
	const Vector2 screen_center_uv = Vector2(0.5f, 0.5f);
	//水滴が画面外へ出ていくときの速度
	constexpr float screen_out_speed = 0.001f;
}

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

	//最大水滴個数分の水滴を生む
	for (int i = 0; i < max_water_drop_count; i++)
	{
		RespawnWaterDrop(i);
	}
}

WaterDropEffect::~WaterDropEffect()
{
	DeleteShaderConstantBuffer(m_cbuffWaterDropH);
}

void WaterDropEffect::Update()
{
	for (int i = 0; i < max_water_drop_count; i++)
	{
		//画面の中央から水滴へのベクトルを作る
		Vector2 centerToWaterDrop = Vector2(
			m_pCBuffWaterDrop->waterData[i].pos.x - screen_center_uv.x,
			m_pCBuffWaterDrop->waterData[i].pos.y - screen_center_uv.y
		);
		//正規化
		Vector2 centerToWaterDropDir = centerToWaterDrop.Normalized();
		//水滴の位置を外側へずらしていく
		m_pCBuffWaterDrop->waterData[i].pos += centerToWaterDropDir * screen_out_speed;

		Vector2 pos = m_pCBuffWaterDrop->waterData[i].pos;

		//外に出たらまた新しい水滴を出現させる
		if (pos.x < 0.0f ||
			pos.x > 1.0f ||
			pos.y < 0.0f ||
			pos.y > 1.0f)
		{
			RespawnWaterDrop(i);
		}
	}
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

float WaterDropEffect::GetRandRatio()
{
	//乱数の範囲を0.0f~1.0fとして返す
	//GetRandに渡す値を大きくするほど、返す値が細かくなる
	return static_cast<float>(GetRand(rand_resolution)) / rand_resolution;
}

void WaterDropEffect::RespawnWaterDrop(int index)
{
	//index番目の水滴を生み出す
	//半径を最低半径~最大半径の間でランダムにする
	//先に最低から最大までの幅を求めてそこにランダムな割合をかけて最低を足す
	m_pCBuffWaterDrop->waterData[index].radius = (max_radius - min_radius) * GetRandRatio() + min_radius;
	//位置も設定する
	//シェーダーに位置を渡すときはuvが0~1なので
	//0~1の間でランダムにする
	m_pCBuffWaterDrop->waterData[index].pos = Vector2(GetRandRatio(),GetRandRatio());
}
