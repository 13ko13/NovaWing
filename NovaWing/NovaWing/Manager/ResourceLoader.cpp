#include "ResourceLoader.h"
#include <DxLib.h>
#include <cassert>

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

	//画像を読み込んでハンドルを保存する
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
