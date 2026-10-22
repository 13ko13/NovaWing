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

	/// <summary>
	/// デバッグ表示の有効/無効を設定する
	/// </summary>
	/// <param name="isEnabled">有効:true / 無効:false</param>
	void SetDebugDrawEnabled(bool isEnabled) { m_isDebugDrawEnabled = isEnabled; }

	/// <summary>
	/// プレイヤーが無敵(ダメージを受けない)かどうかを取得する
	/// </summary>
	/// <returns>無敵:true / 通常:false</returns>
	bool IsPlayerInvincible() const { return m_isPlayerInvincible; }

	/// <summary>
	/// プレイヤーの無敵(ダメージを受けない)を設定する
	/// </summary>
	/// <param name="isInvincible">無敵:true / 通常:false</param>
	void SetPlayerInvincible(bool isInvincible) { m_isPlayerInvincible = isInvincible; }

private:
	//デバッグ用の球・文字などをまとめて表示するかどうか
	//(_DEBUGビルドでも、ポーズ画面からこのフラグでさらにON/OFFできるようにする)
	bool m_isDebugDrawEnabled = true;
	//プレイヤーがダメージを受けないようにするか(ImGuiのSystemセクションから切り替える)
	bool m_isPlayerInvincible = false;
};
