#pragma once

class Fade
{
public:
	Fade();
	~Fade();

	void Update();
	void Draw();

	/// <summary>
	/// 指定フレーム数かけてフェードインする
	/// </summary>
	/// <param name="fadeFrame">フェードにかけるフレーム数</param>
	void StartFadeIn(float fadeFrame);

	/// <summary>
	/// 指定フレーム数かけてフェードアウトする
	/// </summary>
	/// <param name="fadeFrame">フェードにかけるフレーム数</param>
	void StartFadeOut(float fadeFrame);

	/// <summary>
	/// フェード中かどうかを取得する
	/// </summary>
	/// <returns>フェード中であればtrue、そうでなければfalse</returns>
	const bool IsFading() const {return m_isFading;}

	/// <summary>
	/// フェードにかけるフレーム数を取得する
	/// </summary>
	/// <returns>フェードにかけるフレーム数</returns>
	const float GetFadeFrame() const { return m_fadeFrame; }

private:
	//フェードの状態
	enum class FadeState
	{
		FadeIn,		//フェードイン中
		FadeOut,	//フェードアウト中
		None,		//フェードなし
	};

	FadeState m_currentState = FadeState::None;	//現在のフェードの状態

	float m_alpha = 0;				//フェードのアルファ値	
	float m_fadeFrame = 0;			//フェードにかけるフレーム数
	float m_currentFrame = 0;		//現在のフレーム数

	//フェード中か
	bool m_isFading = false;
};

