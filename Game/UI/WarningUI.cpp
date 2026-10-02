#define NOMINMAX

#include <DxLib.h>
#include <cassert>
#include <cmath>
#include <algorithm>

#include "WarningUI.h"
#include "Manager/ResourceLoader.h"
#include "Constants/ShaderRegister.h"
#include "Utility/GraphShaderDraw.h"
#include "Main/Application.h"

namespace
{
	//各パーツの中心位置(1920x1080基準)
	//画面の中心
	constexpr float screen_center_x = 960.0f;
	constexpr float screen_center_y = 540.0f;
	//WARNINGの文字の中心Y
	constexpr float text_center_y = 491.0f;
	//サブテキストの中心Y
	constexpr float sub_text_center_y = 671.0f;
	//左右のアイコンの中心X(枠の側面パネルの中)
	constexpr float icon_left_center_x = 255.0f;
	constexpr float icon_right_center_x = 1665.0f;

	//枠が開く(閉じる)のにかけるフレーム
	constexpr int open_frame = 14;

	//開ききってから、最初に文字を素早く点滅させるフレーム
	constexpr int flicker_frame = 12;
	//素早い点滅の切り替え間隔
	constexpr int flicker_interval = 3;
	//文字の点滅の周期
	constexpr int blink_interval = 40;
	//点滅の周期のうち、文字を暗くするフレーム
	constexpr int blink_dim_frame = 10;
	//暗くしたときの文字のアルファ
	constexpr float blink_dim_alpha = 0.3f;

	//サブテキストが出始めるフレーム(開き始めから)
	constexpr int sub_text_start_frame = 22;
	//サブテキストを左から右へワイプするのにかけるフレーム
	constexpr int sub_text_wipe_frame = 20;

	//アイコンが脈打つ周期
	constexpr float icon_pulse_interval = 30.0f;
	//アイコンのアルファの中心と振れ幅
	constexpr float icon_alpha_base = 0.65f;
	constexpr float icon_alpha_range = 0.35f;

	//画面の縁が脈打つ周期
	constexpr float edge_pulse_interval = 40.0f;
	//画面の縁のアルファの最低値と振れ幅
	constexpr float edge_alpha_base = 0.4f;
	constexpr float edge_alpha_range = 0.4f;
	//画面の縁がフェードイン・アウトするフレーム
	constexpr int edge_fade_frame = 12;

	//スキャンラインを入れる周期(タイトル・リザルトと同じ)
	constexpr float scanline_frequency = 280.0f;
	//シェーダにフレームを渡すときに値が大きすぎるので小さくするための値
	constexpr float time_speed = 0.1f;
}

WarningUI::WarningUI() :
	UIBase()
{
	//グリッチシェーダをロード
	m_glitchPSH = LoadPixelShader(L"GlitchPS.pso");
	assert(m_glitchPSH >= 0);

	//シェーダバッファを作成
	m_cbufferGlitch = CreateShaderConstantBuffer(sizeof(GlitchBuffer));
	m_pCBuffGlitchData = static_cast<GlitchBuffer*>(GetBufferShaderConstantBuffer(m_cbufferGlitch));

	//スキャンラインを入れる周期をシェーダに渡す
	m_pCBuffGlitchData->time = 0.0f;
	m_pCBuffGlitchData->scanlineFrequency = scanline_frequency;
	UpdateShaderConstantBuffer(m_cbufferGlitch);
}

WarningUI::~WarningUI()
{
	//シェーダとバッファを解放する
	DeleteShader(m_glitchPSH);
	DeleteShaderConstantBuffer(m_cbufferGlitch);
}

void WarningUI::Start(int totalFrame)
{
	m_isPlaying = true;
	m_frame = 0;
	m_totalFrame = totalFrame;
}

void WarningUI::Update()
{
	if (!m_isPlaying) return;

	//フレームを進める
	m_frame++;

	//シェーダに時間を渡す
	m_pCBuffGlitchData->time = m_frame * time_speed;
	UpdateShaderConstantBuffer(m_cbufferGlitch);

	//閉じ終わったら終了
	if (m_frame >= m_totalFrame)
	{
		m_isPlaying = false;
	}
}

void WarningUI::Draw()
{
	if (!m_isPlaying) return;

	const ResourceLoader& resourceL = ResourceLoader::GetInstance();
	//UIの見た目の大きさをDebug/Releaseで揃えるためのスケール
	float uiScale = Application::GetInstance().GetUIScale();

	//閉じ始めるフレーム
	int closeStartFrame = m_totalFrame - open_frame;

	//枠の開き具合(0:閉じている～1:開ききっている)
	float openProgress = 0.0f;
	if (m_frame < closeStartFrame)
	{
		openProgress = std::min(1.0f, static_cast<float>(m_frame) / open_frame);
	}
	else
	{
		openProgress = std::max(0.0f, static_cast<float>(m_totalFrame - m_frame) / open_frame);
	}
	//動き始めを速く、最後をゆっくりにする
	openProgress = 1.0f - (1.0f - openProgress) * (1.0f - openProgress);

	//グリッチシェーダを適用
	SetUsePixelShader(m_glitchPSH);
	SetShaderConstantBuffer(m_cbufferGlitch, DX_SHADERTYPE_PIXEL, ShaderRegister::glitch_buffer);

	//画面の縁を赤く脈打たせる
	float edgeFade = std::min({
		1.0f,
		static_cast<float>(m_frame) / edge_fade_frame,
		static_cast<float>(m_totalFrame - m_frame) / edge_fade_frame });
	float edgePulse = 0.5f + 0.5f * std::sin(m_frame * DX_TWO_PI_F / edge_pulse_interval);
	float edgeAlpha = (edge_alpha_base + edge_alpha_range * edgePulse) * edgeFade;
	DrawGraphToShaderByCenter(
		screen_center_x * uiScale, screen_center_y * uiScale, uiScale,
		resourceL.GetGraphic(ResourceLoader::GraphicID::WarningEdge),
		edgeAlpha);

	//枠を中心から左右に開く(リザルトのカーテン演出と同じ)
	if (openProgress > 0.0f)
	{
		DrawGraphToShaderByCenter(
			screen_center_x * uiScale, screen_center_y * uiScale, uiScale,
			resourceL.GetGraphic(ResourceLoader::GraphicID::WarningFrame),
			1.0f,
			0.5f + openProgress * 0.5f,
			0.5f - openProgress * 0.5f);
	}

	//中身は枠が開ききってから、閉じ始めるまで出す
	if (m_frame >= open_frame && m_frame < closeStartFrame)
	{
		//開ききってからのフレーム
		int contentFrame = m_frame - open_frame;

		//WARNINGの文字のアルファ
		float textAlpha = 1.0f;
		if (contentFrame < flicker_frame)
		{
			//出始めは素早く点滅させる
			textAlpha = (contentFrame / flicker_interval) % 2 == 0 ? 1.0f : 0.0f;
		}
		else if ((contentFrame - flicker_frame) % blink_interval >= blink_interval - blink_dim_frame)
		{
			//一定周期で暗くする
			textAlpha = blink_dim_alpha;
		}
		DrawGraphToShaderByCenter(
			screen_center_x * uiScale, text_center_y * uiScale, uiScale,
			resourceL.GetGraphic(ResourceLoader::GraphicID::WarningText),
			textAlpha);

		//左右のアイコンを脈打たせる
		int iconH = resourceL.GetGraphic(ResourceLoader::GraphicID::WarningIcon);
		float iconAlpha = icon_alpha_base + icon_alpha_range * std::cos(contentFrame * DX_TWO_PI_F / icon_pulse_interval);
		DrawGraphToShaderByCenter(
			icon_left_center_x * uiScale, screen_center_y * uiScale, uiScale, iconH, iconAlpha);
		DrawGraphToShaderByCenter(
			icon_right_center_x * uiScale, screen_center_y * uiScale, uiScale, iconH, iconAlpha);

		//サブテキストを左から右へワイプして出す
		int subTextFrame = m_frame - sub_text_start_frame;
		if (subTextFrame > 0)
		{
			float wipeProgress = std::min(1.0f, static_cast<float>(subTextFrame) / sub_text_wipe_frame);
			DrawGraphToShaderByCenter(
				screen_center_x * uiScale, sub_text_center_y * uiScale, uiScale,
				resourceL.GetGraphic(ResourceLoader::GraphicID::WarningSubText),
				1.0f, wipeProgress);
		}
	}

	//シェーダを解除
	SetUsePixelShader(-1);
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, ShaderRegister::glitch_buffer);
}
