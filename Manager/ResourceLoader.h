#pragma once
#include <unordered_map>
#include <string>
#include <Windows.h>
#include <vector>
#include <algorithm>

#include "Scene/SceneID.h"

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

		FloatingNormalMap,//浮遊敵の法線マップ
		FloatingEmissionMap,//浮遊敵のエミッションマップ

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

		WarningFrame,//WARNINGの枠
		WarningText,//WARNINGの文字
		WarningSubText,//WARNINGの下の小さい文字
		WarningIcon,//WARNINGの左右のアイコン
		WarningEdge,//WARNING中に画面の縁を赤くする画像

		BossAppearMovie,//ボス登場ムービー(動画もLoadGraphで読み込む)
	};

	//エフェクトの種類
	enum class EffectID : int
	{
		PlayerBullet,//プレイヤーの弾エフェクト
		WormDeath,//ワームの死亡エフェクト
		FloatingDeath,//浮遊敵の死亡エフェクト
		PlayerChargeBullet,//プレイヤーのチャージ弾エフェクト
		ChargeExplosion,//チャージ弾の着弾爆発エフェクト
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
		LeftWingSplash,//左羽の水しぶきエフェクト
		RightWingSplash,//右羽の水しぶきエフェクト
		LeftBarrelRoll,//左バレルロールのエフェクト
		RightBarrelRoll,//右バレルロールのエフェクト
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
		FloatingBoot,//activeになるときの音

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
		Message,//チュートリアルや、メッセージ用のフォント
	};

	//シーンそれぞれが使うリソースの一覧
	struct SceneResources
	{
		std::vector<ModelID> models;
		std::vector<GraphicID> graphics;
		std::vector<EffectID> effects;
		std::vector<SoundID> sounds;
		std::vector<FontID> fonts;
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

	//サウンドが読み込まれているか
	bool IsSoundLoaded(SoundID id) const;

	//シーン切り替え時のリソースの入れ替え
	//prevにあってnextにないものは解放、nextにしかないものは読み込む
	void OnSceneChange(SceneID prev, SceneID next);

	//非同期ロードを開始する
	void BeginAsyncLoad();
	//非同期ロードを終了する
	void EndAsyncLoad();
	//非同期ロードが進行中か
	bool IsAsyncLoading() const;

	//wstringをModelIDに変換する
	static ResourceLoader::ModelID WStringToModelID(const std::wstring id);

private:
	//コンストラクタ
	ResourceLoader();
	//=defaultでデフォルトデストラクタを生成する
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

	//モデルをロード
	void LoadModel(ModelID id);
	//モデルを解放
	void ReleaseModel(ModelID id);

	//画像をロード
	void LoadGraphic(GraphicID id);
	//画像を開放
	void ReleaseGraphic(GraphicID id);

	//エフェクトをロード
	void LoadEffect(EffectID id);
	//エフェクトを解放
	void ReleaseEffect(EffectID id);

	//サウンドをロード
	void LoadSound(SoundID id);
	//サウンドを解放
	void ReleaseSound(SoundID id);

	//フォントをロード
	void LoadFont(FontID id);
	//フォントを解放
	void ReleaseFont(FontID id);

	//シーンごとのリソース一覧を組み立てる
	void InitSceneResources();

	//シーンが切り替わる際のリソースを変更
	//前のシーンにあって次のシーンに必要ないものは削除
	//前のシーンにあって次のシーンにないものは読み込む
	template<typename IDType>
	void ChangeResources(
		const std::vector<IDType>& prevList,
		const std::vector<IDType>& nextList,
		void(ResourceLoader::* loadFunc)(IDType),
		void(ResourceLoader::*releaseFunc)(IDType)
	)
	{
		//prevにあってnextにないものは解放
		for(const IDType& id : prevList)
		{
			//idをnextListから探して、なかったら解放
			if (std::find(nextList.begin(), nextList.end(), id) == nextList.end())
			{
				//引数として受け取ったリソース解放の関数ポインタに
				//idを渡して、解放させる
				(this->*releaseFunc)(id);
			}
		}
		//nextにあってprevにないものは読み込み
		for(const IDType& id : nextList)
		{
			//idをprevListから探して、なかったら読み込み
			if(std::find(prevList.begin(),prevList.end(),id) == prevList.end())
			{
				//引数として受け取ったリソース読み込みの関数ポインタに
				//idを渡して、読み込みさせる
				(this->*loadFunc)(id);
			}
		}
	}

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
	//フォントのハンドルを保存するマップ
	std::unordered_map<FontID, FontData> m_fontHandles;

	//シーンごとに使うリソースの一覧(初期化時にまとめて書く)
	std::unordered_map<SceneID, SceneResources> m_sceneResources;
};