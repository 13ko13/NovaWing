#include <DxLib.h>
#include <cassert>
#include <EffekseerForDXLib.h>
#include <unordered_map>

#include "ResourceLoader.h"

namespace
{
	//---------- モデルのパス ----------
	constexpr const wchar_t* player_model_path = L"Data/Model/Player.mv1";
	constexpr const wchar_t* floating_enemy_model_path = L"Data/Model/Drone_fix.mv1";
	constexpr const wchar_t* worm_head_model_path = L"Data/Model/Worm_fix.mv1";
	constexpr const wchar_t* rock1_model_path = L"Data/Model/Rock1.mv1";
	constexpr const wchar_t* rock2_model_path = L"Data/Model/Rock2.mv1";
	constexpr const wchar_t* rock3_model_path = L"Data/Model/Rock3.mv1";
	constexpr const wchar_t* stage_model_path = L"Data/Model/Stage.mv1";
	constexpr const wchar_t* boss_model_path = L"Data/Model/Boss.mv1";

	//---------- 画像のパス ----------
	//プレイヤー
	constexpr const wchar_t* player_normal_map_path = L"Data/Model/Player.fbm/StarSparrow_Normal.png";
	constexpr const wchar_t* player_metalic_map_path = L"Data/Model/Player.fbm/StarSparrow_MetallicSmoothness.png";
	constexpr const wchar_t* player_emission_map_path = L"Data/Model/Player.fbm/StarSparrow_Emission.png";
	//浮遊敵
	constexpr const wchar_t* enemy_normal_map_path = L"Data/Model/Drone.fbm/Drone_NormalMap.png";
	constexpr const wchar_t* enemy_emission_map_path = L"Data/Model/Drone.fbm/Drone_Emission.png";
	//ワーム
	constexpr const wchar_t* worm_head_normal_map_path = L"Data/Model/Worm_fix.fbm/Worm_Normal.png";
	constexpr const wchar_t* worm_head_metalic_map_path = L"Data/Model/Worm_fix.fbm/Worm_Metallic.png";
	constexpr const wchar_t* worm_head_emission_map_path = L"Data/Model/Worm_fix.fbm/Worm_Emission_Orange.png";
	constexpr const wchar_t* worm_body_diffuse_map_path = L"Data/Model/Worm_fix.fbm/T_Space_Drone_D_Blue.png";
	//レティクル
	constexpr const wchar_t* normal_reticle_path = L"Data/Image/NormalReticle.png";
	constexpr const wchar_t* charge_reticle_path = L"Data/Image/ChargeReticle.png";
	//スカイボックス
	constexpr const wchar_t* skybox_front_path = L"Data/Image/SkyBox/skybox_front.png";
	constexpr const wchar_t* skybox_back_path = L"Data/Image/SkyBox/skybox_back.png";
	constexpr const wchar_t* skybox_right_path = L"Data/Image/SkyBox/skybox_right.png";
	constexpr const wchar_t* skybox_left_path = L"Data/Image/SkyBox/skybox_left.png";
	constexpr const wchar_t* skybox_up_path = L"Data/Image/SkyBox/skybox_up.png";
	constexpr const wchar_t* skybox_bottom_path = L"Data/Image/SkyBox/skybox_bottom.png";
	//岩
	constexpr const wchar_t* rock_normal_map_path = L"Data/Model/Rock.fbm/rocks_nm_new.png";
	//タイトル
	constexpr const wchar_t* title_logo_path = L"Data/Image/title_logo.png";
	constexpr const wchar_t* game_start_path = L"Data/Image/SelectFrame/Game_Start.png";
	constexpr const wchar_t* game_end_path = L"Data/Image/SelectFrame/Game_End.png";
	constexpr const wchar_t* game_start_on_cursor_path = L"Data/Image/SelectFrame/Game_Start_OnCursor.png";
	constexpr const wchar_t* game_end_on_cursor_path = L"Data/Image/SelectFrame/Game_End_OnCursor.png";
	constexpr const wchar_t* select_background_path = L"Data/Image/SelectFrame/Select_BackGround.png";
	//海
	constexpr const wchar_t* caustics_path = L"Data/Image/Caustics.png";
	//ノイズ
	constexpr const wchar_t* dissolve_noise_path = L"Data/Image/Noise.png";
	//HPゲージ
	constexpr const wchar_t* hp_frame_path = L"Data/Image/HPGauge/HP_Frame_fix.png";
	constexpr const wchar_t* hp_gauge_path = L"Data/Image/HPGauge/HP_Gauge_fix.png";
	//ボス
	constexpr const wchar_t* boss_normal_path = L"Data/Model/Boss.fbm/T_Mech_LOD2_N.png";
	constexpr const wchar_t* boss_emission_path = L"Data/Model/Boss.fbm/T_Mech_LOD2_E.png";

	//---------- エフェクトのパス ----------
	constexpr const wchar_t* player_bullet_effect_path = L"Data/Effect/PlayerBullet/PlayerBullet.efk";
	constexpr const wchar_t* worm_death_effect_path = L"Data/Effect/Exprosion/Exprosion.efk";
	constexpr const wchar_t* floating_death_effect_path = L"Data/Effect/Exprosion2/Exprosion2.efk";
	constexpr const wchar_t* player_charge_bullet_effect_path = L"Data/Effect/PlayerChargeBullet/PlayerChargeBullet.efk";
	constexpr const wchar_t* charging_effect_path = L"Data/Effect/Charging/Charging.efk";
	constexpr const wchar_t* enemy_bullet_effect_path = L"Data/Effect/EnemyBullet/EnemyBullet.efk";
	//水しぶき
	constexpr const wchar_t* splash_effect_path = L"Data/Effect/Splash/Splash.efk";

	//---------- CSV上でのモデル識別文字列 ----------
	constexpr const wchar_t* rock1_csv_name = L"Rock1";
	constexpr const wchar_t* rock2_csv_name = L"Rock2";
	constexpr const wchar_t* rock3_csv_name = L"Rock3";
	constexpr const wchar_t* floating_enemy_csv_name = L"FloatingEnemy";
	constexpr const wchar_t* worm_head_csv_name = L"WormHead";
	constexpr const wchar_t* boss_csv_name = L"Boss";

	//---------- エフェクトの再生スケール ----------
	constexpr float player_bullet_effect_scale = 2.0f;
	constexpr float worm_death_effect_scale = 3.0f;
	constexpr float floating_death_effect_scale = 1.5f;
	constexpr float player_charge_bullet_effect_scale = 1.0f;
	constexpr float charging_effect_scale = 1.0f;
	constexpr float enemy_bullet_effect_scale = 1.5f;
	constexpr float splash_effect_scale = 3.0f;
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
		{rock1_csv_name,ModelID::Rock1},
		{rock2_csv_name,ModelID::Rock2},
		{rock3_csv_name,ModelID::Rock3},
		{floating_enemy_csv_name,ModelID::FloatingEnemy},
		{worm_head_csv_name,ModelID::WormHead},
		{boss_csv_name,ModelID::Boss},
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
	handle = MV1LoadModel(player_model_path);//プレイヤー
	assert(handle >= 0);
	m_modelHandles[ResourceLoader::ModelID::Player] = handle;

	handle = MV1LoadModel(floating_enemy_model_path);//浮遊する敵
	assert(handle >= 0);
	m_modelHandles[ResourceLoader::ModelID::FloatingEnemy] = handle;

	handle = MV1LoadModel(worm_head_model_path);//ワーム
	assert(handle >= 0);
	m_modelHandles[ResourceLoader::ModelID::WormHead] = handle;

	handle = MV1LoadModel(rock1_model_path);//岩1
	assert(handle >= 0);
	m_modelHandles[ResourceLoader::ModelID::Rock1] = handle;

	handle = MV1LoadModel(rock2_model_path);//岩2
	assert(handle >= 0);
	m_modelHandles[ResourceLoader::ModelID::Rock2] = handle;

	handle = MV1LoadModel(rock3_model_path);//岩3
	assert(handle >= 0);
	m_modelHandles[ResourceLoader::ModelID::Rock3] = handle;

	handle = MV1LoadModel(stage_model_path);//ステージ
	assert(handle >= 0);
	m_modelHandles[ResourceLoader::ModelID::Stage] = handle;

	handle = MV1LoadModel(boss_model_path);//ボス
	assert(handle >= 0);
	m_modelHandles[ResourceLoader::ModelID::Boss] = handle;
}

void ResourceLoader::KeepGraph()
{
	int handle = -1;
	//法線マップ
	handle = LoadGraph(player_normal_map_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::PlayerNormalMap] = handle;

	//メタリックマップ
	handle = LoadGraph(player_metalic_map_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::PlayerMetalicMap] = handle;

	//エミッションマップ
	handle = LoadGraph(player_emission_map_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::PlayerEmissionMap] = handle;

	//法線マップ
	handle = LoadGraph(enemy_normal_map_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::EnemyNormalMap] = handle;

	//エミッションマップ
	handle = LoadGraph(enemy_emission_map_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::EnemyEmissionMap] = handle;

	//ワームの頭の法線マップ
	handle = LoadGraph(worm_head_normal_map_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::WormHeadNormalMap] = handle;

	//ワームの頭のメタリックマップ
	handle = LoadGraph(worm_head_metalic_map_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::WormHeadMetalicMap] = handle;

	//ワームの頭のエミッションマップ
	handle = LoadGraph(worm_head_emission_map_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::WormHeadEmissionMap] = handle;

	//ワームの胴体のディフューズマップ
	handle = LoadGraph(worm_body_diffuse_map_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::WormBodyDiffuseMap] = handle;

	//ノーマルレティクル
	handle = LoadGraph(normal_reticle_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::NormalReticle] = handle;

	//チャージレティクル
	handle = LoadGraph(charge_reticle_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::ChargeReticle] = handle;

	//スカイボックス前
	handle = LoadGraph(skybox_front_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::SkyBoxFront] = handle;
	//スカイボックス後
	handle = LoadGraph(skybox_back_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::SkyBoxBack] = handle;
	//スカイボックス右
	handle = LoadGraph(skybox_right_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::SkyBoxRight] = handle;
	//スカイボックス左
	handle = LoadGraph(skybox_left_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::SkyBoxLeft] = handle;
	//スカイボックス上
	handle = LoadGraph(skybox_up_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::SkyBoxUp] = handle;
	//スカイボックス下
	handle = LoadGraph(skybox_bottom_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::SkyBoxBottom] = handle;

	//岩の法線マップテクスチャ
	handle = LoadGraph(rock_normal_map_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::RockNorm] = handle;

	//タイトルロゴ
	handle = LoadGraph(title_logo_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::TitleLogo] = handle;

	//ゲーム開始選択肢
	handle = LoadGraph(game_start_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::GameStart] = handle;

	//ゲーム終了選択肢
	handle = LoadGraph(game_end_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::GameEnd] = handle;

	//カーソルが乗っているときのゲーム開始選択肢
	handle = LoadGraph(game_start_on_cursor_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::GameStartOnCursor] = handle;

	//カーソルが乗っているときのゲーム終了選択肢
	handle = LoadGraph(game_end_on_cursor_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::GameEndOnCursor] = handle;

	//選択肢の背景
	handle = LoadGraph(select_background_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::SelectBackGround] = handle;

	//コースティクス効果用のテクスチャ
	handle = LoadGraph(caustics_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::Caustics] = handle;

	//ディゾルブ用のノイズテクスチャ
	handle = LoadGraph(dissolve_noise_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::DissolveNoise] = handle;

	//HPの枠
	handle = LoadGraph(hp_frame_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::HPFrame] = handle;

	//HPゲージ
	handle = LoadGraph(hp_gauge_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::HPGauge] = handle;

	//ボスのエミッションマップ
	handle = LoadGraph(boss_emission_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::BossEmission] = handle;
	//ボスの法線マップ
	handle = LoadGraph(boss_normal_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::BossNormal] = handle;
}

void ResourceLoader::KeepEffect()
{
	//Effekseerのエフェクトをロードする
	//プレイヤーの弾
	int handle = LoadEffekseerEffect(player_bullet_effect_path,player_bullet_effect_scale);
	assert(handle >= 0);
	m_effectHandles[ResourceLoader::EffectID::PlayerBullet] = handle;

	//ワームエネミーの死亡エフェクト
	handle = LoadEffekseerEffect(worm_death_effect_path,worm_death_effect_scale);
	assert(handle >= 0);
	m_effectHandles[ResourceLoader::EffectID::WormDeath] = handle;

	//浮遊エネミーの死亡エフェクト
	handle = LoadEffekseerEffect(floating_death_effect_path, floating_death_effect_scale);
	assert(handle >= 0);
	m_effectHandles[ResourceLoader::EffectID::FloatingDeath] = handle;

	//プレイヤーのチャージ弾エフェクト
	handle = LoadEffekseerEffect(player_charge_bullet_effect_path, player_charge_bullet_effect_scale);
	assert(handle >= 0);
	m_effectHandles[ResourceLoader::EffectID::PlayerChargeBullet] = handle;

	//プレイヤーのチャージ中エフェクト
	handle = LoadEffekseerEffect(charging_effect_path, charging_effect_scale);
	assert(handle >= 0);
	m_effectHandles[ResourceLoader::EffectID::Charging] = handle;

	//エネミーの弾エフェクト
	handle = LoadEffekseerEffect(enemy_bullet_effect_path, enemy_bullet_effect_scale);
	assert(handle >= 0);
	m_effectHandles[ResourceLoader::EffectID::EnemyBullet] = handle;

	//水しぶきエフェクト
	handle = LoadEffekseerEffect(splash_effect_path, splash_effect_scale);
	assert(handle >= 0);
	m_effectHandles[ResourceLoader::EffectID::Splash] = handle;
}
