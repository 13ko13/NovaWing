#pragma once
#include "Game/UI/UIBase.h"

/// <summary>
/// ボス出現前に表示するWARNINGのUI
/// Start()で表示を始め、指定したフレームが経過すると閉じて終了する
/// </summary>
class WarningUI : public UIBase
{
public:
	WarningUI();
	virtual ~WarningUI();

	void Update() override;
	void Draw() override;

	/// <summary>
	/// 表示を開始する
	/// </summary>
	/// <param name="totalFrame">枠が開き始めてから閉じ終わるまでのフレーム数</param>
	void Start(int totalFrame);

	//表示中か(開き始めてから閉じ終わるまで)
	bool IsPlaying() const { return m_isPlaying; }

private:
	//表示中か
	bool m_isPlaying = false;
	//表示を始めてからのフレーム
	int m_frame = 0;
	//開き始めてから閉じ終わるまでのフレーム数
	int m_totalFrame = 0;

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
};
