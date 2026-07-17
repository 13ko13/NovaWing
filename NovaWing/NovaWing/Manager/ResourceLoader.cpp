#include <DxLib.h>
#include <cassert>
#include <EffekseerForDXLib.h>
#include <unordered_map>

#include "ResourceLoader.h"

namespace
{
	
}

ResourceLoader& ResourceLoader::GetInstance()
{
	//staticでインスタンスを宣言してそれを返す
	static ResourceLoader instance;
	return instance;
}

void ResourceLoader::LoadAll()
{
	//モデルを読み込んでハンドルを保存する
	KeepModel();

	//画像を読み込んでハンドルを保存する
	KeepGraph();

	//エフェクトを読み込んでハンドルを保存する
	KeepEffect();
}

void ResourceLoader::ReleaseAll()
{
	//すべてのリソースを解放する
	//モデル
	for (auto& modelH : m_modelHandles)
	{
		MV1DeleteModel(modelH.second);
	}
	//グラフィック
	for (auto& graphH : m_graphicHandles)
	{
		DeleteGraph(graphH.second);
	}
	//エフェクト
	for (auto& effectH : m_effectHandles)
	{
		DeleteEffekseerEffect(effectH.second);
	}
}

int ResourceLoader::GetModel(ResourceLoader::ModelID id) const
{
	//IDをもとにハンドルを返す
	auto it = m_modelHandles.find(id);
	
	//it != m_modelHandles.end()は、idに対応するハンドルが見つかったかどうかをチェックしている
	//end()は、マップの最後を指すイテレータで、find()が見つからなかったときに返される
	if (it != m_modelHandles.end())
	{
		return it->second;
	}
	else
	{
		assert(false && "モデルIDが見つかりません");
		return -1;
	}
}

int ResourceLoader::GetGraphic(GraphicID id) const
{
	//IDをもとにハンドルを返す
	auto it = m_graphicHandles.find(id);

	if(it != m_graphicHandles.end())
	{
		return it->second;
	}
	else
	{
		assert(false && "グラフィックIDが見つかりません");
		return -1;
	}
}

int ResourceLoader::GetEffect(EffectID id) const
{
	auto it = m_effectHandles.find(id);

	if(it != m_effectHandles.end())
	{
		return it->second;
	}
	else
	{
		assert(false && "エフェクトIDが見つかりません");
		return -1;
	}
}

int ResourceLoader::GetSound(SoundID id) const
{
	auto it = m_soundHandles.find(id);

	if (it != m_soundHandles.end())
	{
		return it->second;
	}
	else
	{
		assert(false && "サウンドIDが見つかりません");
		return -1;
	}
}

ResourceLoader::ModelID ResourceLoader::WStringToModelID(const std::wstring id)
{
	//wstringとmodelIDの対応表を作成
	static std::unordered_map <std::wstring, ResourceLoader::ModelID> table =
	{
		{L"Rock1",ModelID::Rock1},
		{L"Rock2",ModelID::Rock2},
		{L"Rock3",ModelID::Rock3},
		{L"FloatingEnemy",ModelID::FloatingEnemy},
		{L"WormHead",ModelID::WormHead},
	};
	//受け取ったidを使ってtableからそのモデルのIDを受け取る
	auto it = table.find(id);
	if (it != table.end())//見つかった場合
	{
		return it->second;
	}
	else
	{
		//見つからなかった場合はクラッシュ
		assert(false && L"そのモデルIDは見つかりません");
		//Rock1を返す
		return ModelID::Rock1;
	}
}

void ResourceLoader::KeepModel()
{
	int handle = -1;
	handle = MV1LoadModel(L"Data/Model/Player.mv1");//プレイヤー
	assert(handle >= 0); 
	m_modelHandles[ResourceLoader::ModelID::Player] = handle;

	handle = MV1LoadModel(L"Data/Model/Drone_fix.mv1");//浮遊する敵
	assert(handle >= 0); 
	m_modelHandles[ResourceLoader::ModelID::FloatingEnemy] = handle;

	handle = MV1LoadModel(L"Data/Model/Worm_fix.mv1");//ワーム
	assert(handle >= 0); 
	m_modelHandles[ResourceLoader::ModelID::WormHead] = handle;
	
	handle = MV1LoadModel(L"Data/Model/Rock1.mv1");//岩1
	assert(handle >= 0); 
	m_modelHandles[ResourceLoader::ModelID::Rock1] = handle; 

	handle = MV1LoadModel(L"Data/Model/Rock2.mv1");//岩2
	assert(handle >= 0); 
	m_modelHandles[ResourceLoader::ModelID::Rock2] = handle; 

	handle = MV1LoadModel(L"Data/Model/Rock3.mv1");//岩3
	assert(handle >= 0); 
	m_modelHandles[ResourceLoader::ModelID::Rock3] = handle;
	
	handle = MV1LoadModel(L"Data/Model/Stage.mv1");//ステージ
	assert(handle >= 0); 
	m_modelHandles[ResourceLoader::ModelID::Stage] = handle; 
}

void ResourceLoader::KeepGraph()
{
	int handle = -1;
	//法線マップ
	handle = LoadGraph(L"Data/Model/Player.fbm/StarSparrow_Normal.png");
	assert(handle >= 0); 
	m_graphicHandles[ResourceLoader::GraphicID::PlayerNormalMap] = handle;

	//メタリックマップ
	handle = LoadGraph(L"Data/Model/Player.fbm/StarSparrow_MetallicSmoothness.png");
	assert(handle >= 0); 
	m_graphicHandles[ResourceLoader::GraphicID::PlayerMetalicMap] = handle;

	//エミッションマップ
	handle = LoadGraph(L"Data/Model/Player.fbm/StarSparrow_Emission.png");
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::PlayerEmissionMap] = handle;

	//法線マップ
	handle = LoadGraph(L"Data/Model/Drone.fbm/Drone_NormalMap.png");
	assert(handle >= 0); 
	m_graphicHandles[ResourceLoader::GraphicID::EnemyNormalMap] = handle;

	//エミッションマップ
	handle = LoadGraph(L"Data/Model/Drone.fbm/Drone_Emission.png");
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::EnemyEmissionMap] = handle;

	//ワームの頭の法線マップ
	handle = LoadGraph(L"Data/Model/Worm_fix.fbm/Worm_Normal.png");
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::WormHeadNormalMap] = handle;

	//ワームの頭のメタリックマップ
	handle = LoadGraph(L"Data/Model/Worm_fix.fbm/Worm_Metallic.png");
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::WormHeadMetalicMap] = handle;

	//ワームの頭のエミッションマップ
	handle = LoadGraph(L"Data/Model/Worm_fix.fbm/Worm_Emission_Orange.png");
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::WormHeadEmissionMap] = handle;

	//ワームの胴体のディフューズマップ
	handle = LoadGraph(L"Data/Model/Worm_fix.fbm/T_Space_Drone_D_Blue.png");
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::WormBodyDiffuseMap] = handle;

	//ノーマルレティクル
	handle = LoadGraph(L"Data/Image/NormalReticle.png");
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::NormalReticle] = handle;

	//チャージレティクル
	handle = LoadGraph(L"Data/Image/ChargeReticle.png");
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::ChargeReticle] = handle;

	//スカイボックス前
	handle = LoadGraph(L"Data/Image/SkyBox/skybox_front.png");
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::SkyBoxFront] = handle;
	//スカイボックス後
	handle = LoadGraph(L"Data/Image/SkyBox/skybox_back.png");
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::SkyBoxBack] = handle;
	//スカイボックス右
	handle = LoadGraph(L"Data/Image/SkyBox/skybox_right.png");
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::SkyBoxRight] = handle;
	//スカイボックス左
	handle = LoadGraph(L"Data/Image/SkyBox/skybox_left.png");
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::SkyBoxLeft] = handle;
	//スカイボックス上
	handle = LoadGraph(L"Data/Image/SkyBox/skybox_up.png");
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::SkyBoxUp] = handle;
	//スカイボックス下
	handle = LoadGraph(L"Data/Image/SkyBox/skybox_bottom.png");
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::SkyBoxBottom] = handle;

	//岩の法線マップテクスチャ
	handle = LoadGraph(L"Data/Model/Rock.fbm/rocks_nm_new.png");
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::RockNorm] = handle;

	//タイトルロゴ
	handle = LoadGraph(L"Data/Image/title_logo.png");
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::TitleLogo] = handle;

	//ゲーム開始選択肢
	handle = LoadGraph(L"Data/Image/SelectFrame/Game_Start.png");
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::GameStart] = handle;

	//ゲーム終了選択肢
	handle = LoadGraph(L"Data/Image/SelectFrame/Game_End.png");
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::GameEnd] = handle;

	//カーソルが乗っているときのゲーム開始選択肢
	handle = LoadGraph(L"Data/Image/SelectFrame/Game_Start_OnCursor.png");
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::GameStartOnCursor] = handle;

	//カーソルが乗っているときのゲーム終了選択肢
	handle = LoadGraph(L"Data/Image/SelectFrame/Game_End_OnCursor.png");
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::GameEndOnCursor] = handle;

	//選択肢の背景
	handle = LoadGraph(L"Data/Image/SelectFrame/Select_BackGround.png");
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::SelectBackGround] = handle;

	//コースティクス効果用のテクスチャ
	handle = LoadGraph(L"Data/Image/Caustics.png");
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::Caustics] = handle;
}

void ResourceLoader::KeepEffect()
{
	//Effekseerのエフェクトをロードする
	//プレイヤーの弾
	int handle = LoadEffekseerEffect(L"Data/Effect/PlayerBullet/PlayerBullet.efk",1.0f);
	assert(handle >= 0);
	m_effectHandles[ResourceLoader::EffectID::PlayerBullet] = handle;

	//ワームエネミーの死亡エフェクト
	handle = LoadEffekseerEffect(L"Data/Effect/Exprosion/Exprosion.efk",3.0f);
	assert(handle >= 0);
	m_effectHandles[ResourceLoader::EffectID::WormDeath] = handle;

	//浮遊エネミーの死亡エフェクト
	handle = LoadEffekseerEffect(L"Data/Effect/Exprosion2/Exprosion2.efk", 1.5f);
	assert(handle >= 0);
	m_effectHandles[ResourceLoader::EffectID::FloatingDeath] = handle;

	//プレイヤーのチャージ弾エフェクト
	handle = LoadEffekseerEffect(L"Data/Effect/PlayerChargeBullet/PlayerChargeBullet.efk", 1.0f);
	assert(handle >= 0);
	m_effectHandles[ResourceLoader::EffectID::PlayerChargeBullet] = handle;

	//プレイヤーのチャージ中エフェクト
	handle = LoadEffekseerEffect(L"Data/Effect/Charging/Charging.efk", 1.0f);
	assert(handle >= 0);
	m_effectHandles[ResourceLoader::EffectID::Charging] = handle;
}
