#include "Fade.h"
#include "../Main/Application.h"
#include <DxLib.h>

Fade::Fade()
{
}

Fade::~Fade()
{
}

void Fade::Update()
{
	//状態によってアルファ値を更新する
	switch (m_currentState)
	{
	case Fade::FadeState::FadeIn:
		m_currentFrame--;
		if (m_currentFrame <= 0)
		{
			m_currentState = FadeState::None;
			m_isFading = false;
		}
		break;
	case Fade::FadeState::FadeOut:
		m_currentFrame++;
		if (m_currentFrame >= m_fadeFrame)
		{
			m_currentState = FadeState::None;
			m_isFading = false;
		}
		break;
	case Fade::FadeState::None:
		break;
	default:
		break;
	}
}

void Fade::Draw()
{
	//アルファ値をもとにフェード用の四角形を描画する
	switch (m_currentState)
	{
	case Fade::FadeState::FadeIn:
	{
		//ウィンドウに黒い四角形を描画するためにウィンドウサイズを取得する
		const Size& windowSize = Application::GetInstance().GetWindowSize();
		//フェードのアルファ割合を計算する
		m_alpha = m_currentFrame / m_fadeFrame;

		//アルファ値をもとにウィンドウ全体に黒い四角形を描画する
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(m_alpha * 255));
		//黒い四角形描画する
		DrawBox(0, 0, windowSize.m_width, windowSize.m_height, GetColor(0, 0, 0), TRUE);
		//描画後はブレンドモードを元に戻す
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		break;
	}
	case Fade::FadeState::FadeOut:
	{
		//ウィンドウに黒い四角形を描画するためにウィンドウサイズを取得する
		const Size& windowSize = Application::GetInstance().GetWindowSize();
		//フェードのアルファ割合を計算する
		m_alpha = 1.0f - static_cast<float>(m_currentFrame) / m_fadeFrame;

		//アルファ値をもとにウィンドウ全体に黒い四角形を描画する
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>((1.0f - m_alpha) * 255));
		//黒い四角形描画する
		DrawBox(0, 0, windowSize.m_width, windowSize.m_height, GetColor(0, 0, 0), TRUE);
		//描画後はブレンドモードを元に戻す
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		break;
	}
	case Fade::FadeState::None:
		break;
	default:
		break;
	}
}

void Fade::StartFadeIn(float fadeFrame)
{
	//フェード中なら処理を飛ばす
	if (m_isFading) return;

	//フェードインの状態にする
	m_currentState = FadeState::FadeIn;
	//フェードにかけるフレーム数をセットする
	m_currentFrame = fadeFrame;
	m_fadeFrame = fadeFrame;
	//フェード中フラグを立てる
	m_isFading = true;
}

void Fade::StartFadeOut(float fadeFrame)
{
	//フェード中なら処理を飛ばす
	if (m_isFading) return;

	//フェードアウトの状態にする
	m_currentState = FadeState::FadeOut;
	//フェードにかけるフレーム数をセットする
	m_fadeFrame = fadeFrame;
	m_currentFrame = 0;
	//フェード中フラグを立てる
	m_isFading = true;
}
