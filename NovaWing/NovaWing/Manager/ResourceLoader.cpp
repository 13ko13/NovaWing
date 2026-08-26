#include <cassert>
#include <EffekseerForDXLib.h>
#include <unordered_map>

#include "ResourceLoader.h"
#include "Constants/ResourceConstants.h"


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

	//フォントを読み込んでハンドルを保存
	KeepFont();

	//サウンドを読み込んでハンドルを保存
	KeepSound();
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
	//フォント
	for (auto& fontH : m_fontHandles)
	{
		DeleteFontToHandle(fontH.second.handle);
		RemoveFontResourceEx(fontH.second.path, FR_PRIVATE, NULL);
	}
	//サウンド
	for (auto& soundH : m_soundHandles)
	{
		DeleteSoundMem(soundH.second);
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

int ResourceLoader::GetFont(FontID id) const
{
	auto it = m_fontHandles.find(id);

	if (it != m_fontHandles.end())
	{
		return it->second.handle;
	}
	else
	{
		assert(false && "フォントIDが見つかりません");
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
	handle = LoadGraph(player_hp_frame_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::PlayerHPFrame] = handle;

	//HPゲージ
	handle = LoadGraph(player_hp_gauge_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::PlayerHPGauge] = handle;

	//ボスのエミッションマップ
	handle = LoadGraph(boss_emission_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::BossEmission] = handle;

	//ボスの法線マップ
	handle = LoadGraph(boss_normal_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::BossNormal] = handle;

	//ボスのHPの枠
	handle = LoadGraph(boss_hp_frame_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::BossHPFrame] = handle;

	//ボスのHPゲージ
	handle = LoadGraph(boss_hp_gauge_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::BossHPGauge] = handle;

	//リザルトテンプレート画像
	handle = LoadGraph(result_templete_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::ResultTemplete] = handle;

	//Aボタンの画像
	handle = LoadGraph(a_button_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::ButtonA] = handle;

	//決定のテキスト画像
	handle = LoadGraph(decide_text_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::DecideText] = handle;

	//決定のテキスト画像
	handle = LoadGraph(next_text_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::NextText] = handle;

	//リトライ選択肢画像
	handle = LoadGraph(retry_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::ReTry] = handle;

	//カーソルが乗っているときのリトライ選択肢画像
	handle = LoadGraph(retry_on_cursor_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::ReTryOnCursor] = handle;

	//タイトルに戻る選択肢画像
	handle = LoadGraph(back_title_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::BackTitle] = handle;

	//カーソルが乗っているときのタイトルに戻る選択肢画像
	handle = LoadGraph(back_title_on_cursor_path);
	assert(handle >= 0);
	m_graphicHandles[ResourceLoader::GraphicID::BackTitleOnCursor] = handle;
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

	//浮遊敵召喚時エフェクト
	handle = LoadEffekseerEffect(summon_floating_eff_path, summon_floating_eff_scale);
	assert(handle >= 0);
	m_effectHandles[ResourceLoader::EffectID::SummonFloating] = handle;

	//ワーム召喚時エフェクト
	handle = LoadEffekseerEffect(summon_worm_eff_path, summon_worm_eff_scale);
	assert(handle >= 0);
	m_effectHandles[ResourceLoader::EffectID::SummonWorm] = handle;

	//ボスのビームエフェクト
	handle = LoadEffekseerEffect(boss_beam_eff_patgh, boss_beam_eff_scale);
	assert(handle >= 0);
	m_effectHandles[ResourceLoader::EffectID::BossBeam] = handle;

	//ボスの無敵エフェクト
	handle = LoadEffekseerEffect(boss_shield_effect_path, boss_shield_eff_scale);
	assert(handle >= 0);
	m_effectHandles[ResourceLoader::EffectID::BossShield] = handle;

	//ヒットエフェクト
	handle = LoadEffekseerEffect(hit_effect_path, hit_effect_scale);
	assert(handle >= 0);
	m_effectHandles[ResourceLoader::EffectID::HitEffect] = handle;

	//ボス死亡エフェクト
	handle = LoadEffekseerEffect(boss_death_eff_path, boss_death_eff_scale);
	assert(handle >= 0);
	m_effectHandles[ResourceLoader::EffectID::BossDeath] = handle;
}

void ResourceLoader::KeepSound()
{
	//タイトルのブースト音
	int handle = LoadSoundMem(title_boost_sound_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::TitleBoost] = handle;
	//決定音
	handle = LoadSoundMem(decision_sound_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::Decision] = handle;
	//選択音
	handle = LoadSoundMem(on_cursor_sound_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::OnCursor] = handle;
	//タイトルロゴ出現時の衝撃音
	handle = LoadSoundMem(title_logo_impact_sound_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::TitleLogoImpact] = handle;
	//タイトルBGM
	handle = LoadSoundMem(title_bgm_sound_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::TitleBGM] = handle;
}

void ResourceLoader::KeepFont()
{
	//リザルト時のフォント
	//フォントをPC内に一時的に追加
	AddFontResourceEx(result_font_path, FR_PRIVATE, NULL);
	int handle = CreateFontToHandle(
		result_font_name,
		result_font_size,
		result_font_thick,
		result_font_type
	);
	assert(handle >= 0);
	SetFontSpaceToHandle(font_space, handle);
	//ttfのパスとハンドルを同時に保存
	m_fontHandles[ResourceLoader::FontID::Result].handle = handle;
	m_fontHandles[ResourceLoader::FontID::Result].path = result_font_path;
}