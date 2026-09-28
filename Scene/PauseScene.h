#pragma once
#include <array>
#include <memory>

#include "Scene.h"

class SoundManager;
class Player;

class PauseScene : public Scene
{
public:
	//pSoundManagerとpPlayerはGameSceneから渡す
	//(ポーズ中も鳴り続けさせるため、PauseScene破棄の影響を受けないようweak_ptrで借りる)
	PauseScene(
		SceneController& controller,
		std::weak_ptr<SoundManager> pSoundManager,
		std::weak_ptr<Player> pPlayer);
	~PauseScene();

	void Init() override;
	void Update() override;
	void Draw() override;

private:
	//選択肢
	enum class Select
	{
		BackGame,//ゲームに戻る
		BackTitle,//タイトルに戻る
#ifdef _DEBUG
		ToggleDebugDraw,//デバッグ表示のON/OFF切り替え(_DEBUGビルドのみ表示)
		Restart,//ゲームを最初からやり直す(_DEBUGビルドのみ表示)
		WarpPlayer,//プレイヤーを指定位置に移動する(_DEBUGビルドのみ表示)
#endif

		Max,//最大
	};
	Select m_select = Select::BackGame;//現在の選択肢

	Select m_prevSelectIdx = Select::BackGame;
	std::array<float, static_cast<size_t>(Select::Max)> m_wipeProgress = {};
	int m_glitchPSH = -1;
	struct GlitchBuffer
	{
		float time;
		float scanlineFrequency;
		float dummy[2];
	};
	int m_cbufferGlitch = -1;
	GlitchBuffer* m_pCBuffGlitchData = nullptr;
	int m_frame = 0;
	int m_backGroundOpenFrame = 0;

	//サウンドマネージャーへのポインタ(GameSceneが所有するものを借りている)
	std::weak_ptr<SoundManager> m_pSoundManager;

#ifdef _DEBUG
	//プレイヤーへのポインタ(GameSceneが所有するものを借りている、位置ワープ機能でのみ使用)
	std::weak_ptr<Player> m_pPlayer;

	//プレイヤー位置ワープ画面を開いているか
	bool m_isPlayerWarpMode = false;
	//物差し上で選んでいる位置の割合(0.0=初期位置,1.0=ボス出現位置)
	float m_warpCursorRatio = 0.0f;

	/// <summary>
	/// プレイヤー位置ワープ画面の更新
	/// </summary>
	void UpdatePlayerWarpMode();

	/// <summary>
	/// プレイヤー位置ワープ画面の描画
	/// </summary>
	void DrawPlayerWarpMode();
#endif
};