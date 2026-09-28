#pragma once

/// <summary>
/// デバッグ表示(デバッグ用の球・文字など)のON/OFFをまとめて管理するシングルトンクラス
/// </summary>
class DebugManager
{
private:
	DebugManager();

public:
	/// <summary>
	/// DebugManagerのインスタンスを取得する
	/// </summary>
	/// <returns></returns>
	static DebugManager& GetInstance();

	//コピーと代入を禁止する(消す)
	DebugManager(const DebugManager&) = delete;
	DebugManager& operator=(const DebugManager&) = delete;

	/// <summary>
	/// デバッグ表示が有効かどうかを取得する
	/// </summary>
	/// <returns>有効:true / 無効:false</returns>
	bool IsDebugDrawEnabled() const { return m_isDebugDrawEnabled; }

	/// <summary>
	/// デバッグ表示の有効/無効を反転させる
	/// </summary>
	void ToggleDebugDrawEnabled() { m_isDebugDrawEnabled = !m_isDebugDrawEnabled; }

private:
	//デバッグ用の球・文字などをまとめて表示するかどうか
	//(_DEBUGビルドでも、ポーズ画面からこのフラグでさらにON/OFFできるようにする)
	bool m_isDebugDrawEnabled = true;
};
