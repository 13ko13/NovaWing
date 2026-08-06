#pragma once
#include <unordered_map>
#include <string>

class ResourceLoader
{
public:
	//モデルの種類
	enum class ModelID : int
	{
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
		GameEndOnCursor,////カーソルが乗っているときのゲーム終了選択肢画像
		SelectBackGround,//選択肢の背景画像

		Caustics,//コースティクス効果用のテクスチャ
		DissolveNoise,//ニアクリップフェード対処用のノイズテクスチャ

		HPFrame,//HPの枠
		HPGauge,//HPゲージ

		BossEmission,//ボスのエミッション
		BossNormal,//ボスの法線マップ
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
	};

	//音の種類
	enum class SoundID :int
	{
		
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

private:
	//IDをいれて直感的にアクセスできるようにするためのマップ
	std::unordered_map<ResourceLoader::ModelID, int> m_modelHandles;//モデルのハンドルを保存するマップ
	std::unordered_map<GraphicID, int> m_graphicHandles;//グラフィックのハンドルを保存するマップ
	std::unordered_map<EffectID, int> m_effectHandles;//エフェクトのハンドルを保存するマップ
	std::unordered_map<SoundID, int> m_soundHandles;//サウンドのハンドルを保存するマップ
};