#pragma once
#include <unordered_map>

class ResourceLoader
{
public:
	//モデルの種類
	enum class ModelID : int
	{
		Player, //プレイヤー
		FloatingEnemy,//浮遊する敵
		WormHead,//ワームの頭
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
	};

	//エフェクトの種類
	enum class EffectID : int
	{
		
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

private:
	//=defaultでデフォルトコンストラクタを生成する
	ResourceLoader() = default;
	//デストラクタも同様
	~ResourceLoader() = default;

	//コピーコンストラクタとコピー代入演算子は削除する
	ResourceLoader(const ResourceLoader&) = delete;
	ResourceLoader& operator=(const ResourceLoader&) = delete;

private:
	//IDをいれて直感的にアクセスできるようにするためのマップ
	std::unordered_map<ResourceLoader::ModelID, int> m_modelHandles;//モデルのハンドルを保存するマップ
	std::unordered_map<GraphicID, int> m_graphicHandles;//グラフィックのハンドルを保存するマップ
	std::unordered_map<EffectID, int> m_effectHandles;//エフェクトのハンドルを保存するマップ
	std::unordered_map<SoundID, int> m_soundHandles;//サウンドのハンドルを保存するマップ
};