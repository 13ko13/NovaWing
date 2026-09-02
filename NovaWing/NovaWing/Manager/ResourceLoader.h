#pragma once
#include <unordered_map>
#include <string>
#include <Windows.h>

class ResourceLoader
{
public:
	//モデルの種類
	enum class ModelID : int
	{
		None,
		Player, //プレイヤー
		FloatingEnemy,//浮遊する敵
		WormHead,//ワームの頭
		Rock1,//岩1
		Rock2,//岩2
		Rock3,//岩3
		Stage,//ステージ
		Boss,//ボス
	};

	//グラフィックの種類
	enum class GraphicID : int
	{
		PlayerNormalMap,//プレイヤーの法線マップ
		PlayerMetalicMap,//プレイヤーのメタリックマップ
		PlayerEmissionMap,//プレイヤーのエミッションマップ

		EnemyNormalMap,//敵の法線マップ
		EnemyEmissionMap,//敵のエミッションマップ

		WormHeadNormalMap,//ワームの頭の法線マップ
		WormHeadMetalicMap,//ワームの頭のメタリックマップ
		WormHeadEmissionMap,//ワームの頭のエミッションマップ
		WormBodyDiffuseMap,//ワームの胴体のディフューズマップ

		NormalReticle,//ノーマル状態のレティクル
		ChargeReticle,//チャージ状態のレティクル

		SkyBoxFront,//スカイボックス(前)
		SkyBoxBack,//スカイボックス(後)
		SkyBoxLeft,//スカイボックス(左)
		SkyBoxRight,//スカイボックス(右)
		SkyBoxUp,//スカイボックス(上)
		SkyBoxBottom,//スカイボックス(下)

		RockNorm,//岩の法線マップ

		TitleLogo,//タイトルロゴ
		GameStart,//ゲーム開始選択肢画像
		GameEnd,//ゲーム終了選択肢画像
		GameStartOnCursor,//カーソルが乗っているときのゲーム開始選択肢画像
		GameEndOnCursor,//カーソルが乗っているときのゲーム終了選択肢画像
		SelectBackGround,//選択肢の背景画像

		Caustics,//コースティクス効果用のテクスチャ
		DissolveNoise,//ニアクリップフェード対処用のノイズテクスチャ

		PlayerHPFrame,//プレイヤーのHPの枠
		PlayerHPGauge,//プレイヤーのHPゲージ

		BossEmission,//ボスのエミッション
		BossNormal,//ボスの法線マップ

		BossHPFrame,//ボスのHPの枠
		BossHPGauge,//ボスのHPゲージ

		ResultTemplete,//リザルトのテンプレート画像
		ButtonA,//Aボタンの画像
		DecideText,//決定のテキスト画像
		NextText,//次へ のテキスト画像
		ReTry,//リトライ選択肢画像
		ReTryOnCursor,//カーソルが乗っているときのリトライ選択肢画像
		BackTitle,//タイトルへ戻る選択肢画像
		BackTitleOnCursor,//カーソルが乗っているときのタイトルへ戻る選択肢画像

		SpecialGaugeFrame,//プレイヤーのスペシャルゲージの枠
		SpecialGauge,//スペシャルゲージ

		BackGame,//ゲームに戻る
		BackGameOnCursor,//カーソルが乗っているときのゲームに戻る
	};

	//エフェクトの種類
	enum class EffectID : int
	{
		PlayerBullet,//プレイヤーの弾エフェクト
		WormDeath,//ワームの死亡エフェクト
		FloatingDeath,//浮遊敵の死亡エフェクト
		PlayerChargeBullet,//プレイヤーのチャージ弾エフェクト
		Charging,//プレイヤーのチャージ中のエフェクト
		EnemyBullet,//エネミーの弾エフェクト
		Splash,//水しぶきエフェクト
		SummonFloating,//浮遊敵召喚時エフェクト
		SummonWorm,//ワーム召喚時エフェクト
		BossBeam,//ボスのビームエフェクト
		BossShield,//ボスのシールドエフェクト
		HitEffect,//被弾時エフェクト(敵味方共通)
		BossDeath,//ボスの死亡エフェクト
		Boost,//ブーストエフェクト
	};

	//音の種類
	enum class SoundID :int
	{
		//タイトル
		TitleBoost,//タイトルでのブースト音
		Decision,//決定音
		OnCursor,//カーソルが選択肢に乗った時の音
		TitleLogoImpact,//タイトルロゴが出現するときの衝撃音
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

		//ボス
		BossMove,//ボスの着地音
		BossBeam,//ボスのビーム発射音
		BossSummon,//ボスの雑魚召喚音
		BossRecovery,//ボスの無敵シールド被弾音
		BossDamage,//ボスの被弾音
		BossDeath,//ボスの死亡音
		BossQuake,//ボス出現前の地震音

		//浮遊敵・ワームエネミー共通
		EnemyDeath,//死亡音(爆発音)
		EnemyShoot,//弾発射音

		//浮遊敵
		EnemyBoot,//activeになるときの音

		//ワームエネミー
		WormMove,//移動音

		//BGM
		GameBGM,//ゲームBGM
		BossBGM,//ボスBGM
		ResultBGM,//リザルトBGM
		GameoverBGM,//ゲームオーバーBGM

		//リザルト
		DataAppear,//カーテン演出音
		ScoreCount,//スコア加算音
	};

	//フォントの種類
	enum class FontID : int
	{
		Result,//リザルト用のフォント
	};

public:
	static ResourceLoader& GetInstance();

	//ロード
	void LoadAll();
	//解放
	void ReleaseAll();

	//取得
	int GetModel(ResourceLoader::ModelID id) const;
	int GetGraphic(GraphicID id) const;
	int GetEffect(EffectID id) const;
	int GetSound(SoundID id) const;
	int GetFont(FontID id) const;

	//wstringをModelIDに変換する
    static ResourceLoader::ModelID WStringToModelID(const std::wstring id);

private:
	//=defaultでデフォルトコンストラクタを生成する
	ResourceLoader() = default;
	//デストラクタも同様
	~ResourceLoader() = default;

	//コピーコンストラクタとコピー代入演算子は削除する
	ResourceLoader(const ResourceLoader&) = delete;
	ResourceLoader& operator=(const ResourceLoader&) = delete;

	//モデルのハンドルをすべて保存する
	void KeepModel();
	//画像のハンドルをすべて保存する
	void KeepGraph();
	//エフェクトのハンドルをすべて保存する
	void KeepEffect();
	//サウンドのハンドルをすべて保存する
	void KeepSound();
	//フォントのハンドルをすべて保存する
	void KeepFont();

private:
	//IDをいれて直感的にアクセスできるようにするためのマップ
	std::unordered_map<ResourceLoader::ModelID, int> m_modelHandles;//モデルのハンドルを保存するマップ
	std::unordered_map<GraphicID, int> m_graphicHandles;//グラフィックのハンドルを保存するマップ
	std::unordered_map<EffectID, int> m_effectHandles;//エフェクトのハンドルを保存するマップ
	std::unordered_map<SoundID, int> m_soundHandles;//サウンドのハンドルを保存するマップ

	//フォントの情報
	struct FontData
	{
		int handle;
		LPCWSTR path;//RemoveFontResourceEXで必要
	};

	std::unordered_map<FontID, FontData> m_fontHandles;//フォントのハンドルを保存するマップ
};