#pragma once

class ModelAnimator
{
public:
	ModelAnimator(int modelHandle);
	~ModelAnimator();

	/// <summary>
	/// アニメーションを再生(変更)
	/// </summary>
	/// <param name="animIndex">再生させるアニメーション番号</param>
	void Play(int animIndex, bool isLoop, float animSpeed = 1.0f, float startFrame = 0.0f);

	/// <summary>
	/// アニメーションを更新
	/// </summary>
	/// <param name="blendTime">何フレームでブレンドするか</param>
	void Update(float blendTime);

	/// <summary>
	/// 前フレームのアニメーションが終わっているかを取得
	/// </summary>
	/// <returns>前フレームのアニメーションが終わっているか</returns>
	bool const IsEnd() const;

	/// <summary>
	/// 今の再生時間を取得
	/// </summary>
	/// <returns>アタッチ中のアニメーションの現在の再生時間</returns>
	float GetPlayTime() const { return m_currentTime; }

	/// <summary>
	/// アニメーションの総再生時間を返す
	/// </summary>
	/// <returns>アタッチ中のアニメーションの総再生時間</returns>
	float GetAnimLength() const;

private:
	//モデルのハンドル
	int m_modelHandle = -1;

	//現在再生中のアタッチ番号
	int m_currentAttachIdx = -1;
	//前フレームで再生されていたアタッチ番号
	int m_prevAttachIdx = -1;

	//現在再生中のアニメーションの番号
	int m_animIdx = -1;

	//現在再生中のアニメーション時間
	float m_currentTime = 0.0f;
	float m_prevTime = 0.0f;

	//アニメーションの速度
	float m_animSpeed = 1.0f;

	//ブレンドするときのタイマー
	float m_blendTimer = 0.0f;

	//現在ブレンド中か
	bool m_isBlending = false;

	//アニメーションが終わったか
	bool m_isEnd = false;

	//アニメーションをループするか
	bool m_isLoop = false;
};