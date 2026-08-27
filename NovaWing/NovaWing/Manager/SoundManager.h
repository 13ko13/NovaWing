#pragma once
#include <string>
#include <map>

#include "Manager/ResourceLoader.h"

class SoundManager
{
public:
	enum class SoundType
	{
		//タイトル
		TitleBoost,//タイトルでのブースト音
		Decision,//決定音
		OnCursor,//カーソルが乗った時の音
		TitleLogoImpact,//タイトルロゴ出現時の衝撃音
		TitleBGM,//タイトルBGM

		//プレイヤー
		NormalShoot,//ノーマルショット
		PlayerDeath,//プレイヤー死亡
		PlayerDamage,//プレイヤーダメージ
		ChargeShoot,//チャージショット
		Brake,//ブレーキ
		Boost,//ブースト
		ChargeComplete,//チャージ完了
		Charging,//チャージ中
		Somersoult,//宙返り
	};

public:
	SoundManager();
	~SoundManager();

	/// <summary>
	/// 初期化
	/// </summary>
	void Init();

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// 音をフェードインさせながら再生する
	/// </summary>
	/// <param name="soundType">再生する音の種類</param>
	/// <param name="duration">フェードインにかける時間</param>
	/// <param name="loop">ループするかどうか</param>
	void PlayFadeIn(SoundType soundType, float duration, bool loop = false);
	
	/// <summary>
	/// 音をフェードアウトさせながら停止する
	/// </summary>
	/// <param name="soundType">停止する音の種類</param>
	/// <param name="duration">フェードアウトにかける時間</param>
	void FadeOut(SoundType soundType, float duration);

	/// <summary>
	/// 再生(ループするとき)
	/// </summary>
	/// <param name="soundType">種類</param>
	/// <param name="loop">ループするかどうか(指定しないとループしない)</param>
	/// <param name="isOnce">既に再生中なら重ねて鳴らさないようにするか</param>
	void Play(SoundType soundType, bool loop = false, bool isOnce = false);

	/// <summary>
	/// 指定の音を停止する
	/// </summary>
	/// <param name="type">音の種類</param>
	void Stop(SoundType type);

	/// <summary>
	/// すべての音を停止させる
	/// </summary>
	void StopAll();

	/// <summary>
	/// 音量を変更
	/// </summary>
	/// <param name="type">音の種類</param>
	/// <param name="volume">音量</param>
	void SetVolume(SoundType type, int volume);

private:
	//データの初期化
	void InitData(
		SoundType type, 
		ResourceLoader::SoundID soundID,
		bool isLoaded, 
		int volume, bool isLoop);

private:
	//フェードの状態
	enum class FadeState
	{
		FadeIn,//フェードイン中
		FadeOut,//フェードアウト中
		None,//フェードしていない
	};

private:
	//音に必要なデータをまとめた構造体
	struct SoundData
	{
		int handle = -1;//サウンドハンドル
		int volume = 255;//0～255
		bool isLoop = false;//デフォルトのループ設定
		bool loaded = false;//ロード済みフラグ

		//フェードに使うデータ
		FadeState fadeState = FadeState::None;//フェードの状態
		int currentVolume = 0;//現在の音量
		int targetVolume = 255;//目標の音量
		float fadeDuration = 0.0f;//フェードにかける時間
		float fadeTimer = 0.0f;//フェードの経過時間
	};

	//SoundTypeをキー、SoundDataを値とするマップ
	std::map<SoundType, SoundData> m_sounds;
};

