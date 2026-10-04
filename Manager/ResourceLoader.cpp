#include <cassert>
#include <EffekseerForDXLib.h>
#include <unordered_map>
#include <cassert>

#include "ResourceLoader.h"
#include "Constants/ResourceConstants.h"

namespace
{
	//GraphicIDとパスの対応表を作成
	const std::unordered_map<ResourceLoader::GraphicID, const wchar_t*> graphic_paths =
	{
		//プレイヤー
		{ ResourceLoader::GraphicID::PlayerNormalMap, player_normal_map_path },//法線マップ
		{ ResourceLoader::GraphicID::PlayerMetalicMap, player_metalic_map_path },//メタリックマップ
		{ ResourceLoader::GraphicID::PlayerEmissionMap, player_emission_map_path },//エミッションマップ

		//浮遊敵
		{ ResourceLoader::GraphicID::EnemyNormalMap, enemy_normal_map_path },//法線マップ
		{ ResourceLoader::GraphicID::EnemyEmissionMap, enemy_emission_map_path },//エミッションマップ

		//ワーム
		{ ResourceLoader::GraphicID::WormHeadNormalMap, worm_head_normal_map_path },//頭の法線マップ
		{ ResourceLoader::GraphicID::WormHeadMetalicMap, worm_head_metalic_map_path },//頭のメタリックマップ
		{ ResourceLoader::GraphicID::WormHeadEmissionMap, worm_head_emission_map_path },//頭のエミッションマップ
		{ ResourceLoader::GraphicID::WormBodyDiffuseMap, worm_body_diffuse_map_path },//胴体のディフューズマップ

		//レティクル
		{ ResourceLoader::GraphicID::NormalReticle, normal_reticle_path },//ノーマルレティクル
		{ ResourceLoader::GraphicID::ChargeReticle, charge_reticle_path },//チャージレティクル

		//スカイボックス
		{ ResourceLoader::GraphicID::SkyBoxFront, skybox_front_path },//前
		{ ResourceLoader::GraphicID::SkyBoxBack, skybox_back_path },//後
		{ ResourceLoader::GraphicID::SkyBoxRight, skybox_right_path },//右
		{ ResourceLoader::GraphicID::SkyBoxLeft, skybox_left_path },//左
		{ ResourceLoader::GraphicID::SkyBoxUp, skybox_up_path },//上
		{ ResourceLoader::GraphicID::SkyBoxBottom, skybox_bottom_path },//下

		//岩
		{ ResourceLoader::GraphicID::RockNorm, rock_normal_map_path },//法線マップ

		//タイトル
		{ ResourceLoader::GraphicID::TitleLogo, title_logo_path },//タイトルロゴ
		{ ResourceLoader::GraphicID::GameStart, game_start_path },//ゲーム開始選択肢
		{ ResourceLoader::GraphicID::GameEnd, game_end_path },//ゲーム終了選択肢
		{ ResourceLoader::GraphicID::GameStartOnCursor, game_start_on_cursor_path },//カーソルが乗っているときのゲーム開始選択肢
		{ ResourceLoader::GraphicID::GameEndOnCursor, game_end_on_cursor_path },//カーソルが乗っているときのゲーム終了選択肢
		{ ResourceLoader::GraphicID::SelectBackGround, select_background_path },//選択肢の背景

		//シェーダー用
		{ ResourceLoader::GraphicID::Caustics, caustics_path },//コースティクス効果用のテクスチャ
		{ ResourceLoader::GraphicID::DissolveNoise, dissolve_noise_path },//ディゾルブ用のノイズテクスチャ

		//プレイヤーのHP
		{ ResourceLoader::GraphicID::PlayerHPFrame, player_hp_frame_path },//HPの枠
		{ ResourceLoader::GraphicID::PlayerHPGauge, player_hp_gauge_path },//HPゲージ

		//ボス
		{ ResourceLoader::GraphicID::BossEmission, boss_emission_path },//エミッションマップ
		{ ResourceLoader::GraphicID::BossNormal, boss_normal_path },//法線マップ
		{ ResourceLoader::GraphicID::BossHPFrame, boss_hp_frame_path },//HPの枠
		{ ResourceLoader::GraphicID::BossHPGauge, boss_hp_gauge_path },//HPゲージ

		//リザルト・ゲームオーバー
		{ ResourceLoader::GraphicID::ResultTemplete, result_templete_path },//リザルトテンプレート画像
		{ ResourceLoader::GraphicID::ButtonA, a_button_path },//Aボタンの画像
		{ ResourceLoader::GraphicID::DecideText, decide_text_path },//決定のテキスト画像
		{ ResourceLoader::GraphicID::NextText, next_text_path },//次へのテキスト画像
		{ ResourceLoader::GraphicID::ReTry, retry_path },//リトライ選択肢画像
		{ ResourceLoader::GraphicID::ReTryOnCursor, retry_on_cursor_path },//カーソルが乗っているときのリトライ選択肢画像
		{ ResourceLoader::GraphicID::BackTitle, back_title_path },//タイトルに戻る選択肢画像
		{ ResourceLoader::GraphicID::BackTitleOnCursor, back_title_on_cursor_path },//カーソルが乗っているときのタイトルに戻る選択肢画像

		//スペシャルゲージ
		{ ResourceLoader::GraphicID::SpecialGaugeFrame, special_gauge_frame_path },//枠
		{ ResourceLoader::GraphicID::SpecialGauge, special_gauge_path },//ゲージ

		//ポーズ
		{ ResourceLoader::GraphicID::BackGame, back_game_path },//ゲームに戻る選択肢の画像
		{ ResourceLoader::GraphicID::BackGameOnCursor, back_game_on_cursor_path },//カーソルが乗っているときのゲームに戻る選択肢の画像

		//WARNING演出
		{ ResourceLoader::GraphicID::WarningFrame, warning_frame_path },//WARNINGの枠
		{ ResourceLoader::GraphicID::WarningText, warning_text_path },//WARNINGの文字
		{ ResourceLoader::GraphicID::WarningSubText, warning_sub_text_path },//WARNINGの下の小さい文字
		{ ResourceLoader::GraphicID::WarningIcon, warning_icon_path },//WARNINGの左右のアイコン
		{ ResourceLoader::GraphicID::WarningEdge, warning_edge_path },//WARNING中に画面の縁を赤くする画像

		//ボス登場ムービー(動画もLoadGraphで読み込み、PlayMovieToGraphで再生する)
		{ ResourceLoader::GraphicID::BossAppearMovie, boss_appear_movie_path },
	};

	//モデルIDとパスの対応表
	const std::unordered_map<ResourceLoader::ModelID, const wchar_t*> model_paths =
	{
		{ ResourceLoader::ModelID::Player, player_model_path },//プレイヤー
		{ ResourceLoader::ModelID::FloatingEnemy, floating_enemy_model_path },//浮遊する敵
		{ ResourceLoader::ModelID::WormHead, worm_head_model_path },//ワーム
		{ ResourceLoader::ModelID::Rock1, rock1_model_path },//岩1
		{ ResourceLoader::ModelID::Rock2, rock2_model_path },//岩2
		{ ResourceLoader::ModelID::Rock3, rock3_model_path },//岩3
		{ ResourceLoader::ModelID::Stage, stage_model_path },//ステージ
		{ ResourceLoader::ModelID::Boss, boss_model_path },//ボス
	};

	//音IDとパスの対応表
	const std::unordered_map<ResourceLoader::SoundID, const wchar_t*> sound_paths =
	{
		//タイトル
		{ ResourceLoader::SoundID::TitleBoost, title_boost_sound_path },//タイトルのブースト音
		{ ResourceLoader::SoundID::Decision, decision_sound_path },//決定音
		{ ResourceLoader::SoundID::OnCursor, on_cursor_sound_path },//選択音
		{ ResourceLoader::SoundID::TitleLogoImpact, title_logo_impact_sound_path },//タイトルロゴ出現時の衝撃音
		{ ResourceLoader::SoundID::TitleBGM, title_bgm_sound_path },//タイトルBGM

		//プレイヤー
		{ ResourceLoader::SoundID::NormalShoot, player_normal_shoot_se_path },//通常ショット
		{ ResourceLoader::SoundID::PlayerDeath, player_death_se_path },//死亡
		{ ResourceLoader::SoundID::PlayerDamage, player_damage_se_path },//ダメージ
		{ ResourceLoader::SoundID::ChargeShoot, player_charge_shoot_se_path },//チャージショット
		{ ResourceLoader::SoundID::Brake, brake_se_path },//ブレーキ
		{ ResourceLoader::SoundID::Boost, boost_se_path },//ブースト
		{ ResourceLoader::SoundID::ChargeComplete, charge_complete_se_path },//チャージ完了
		{ ResourceLoader::SoundID::Charging, charging_se_path },//チャージ中
		{ ResourceLoader::SoundID::Somersoult, somersoult_se_path },//宙返り

		//ボス
		{ ResourceLoader::SoundID::BossMove, boss_move_se_path },//着地音
		{ ResourceLoader::SoundID::BossBeam, boss_beam_se_path },//ビーム発射音
		{ ResourceLoader::SoundID::BossSummon, boss_summon_se_path },//雑魚召喚音
		{ ResourceLoader::SoundID::BossRecovery, boss_recovery_se_path },//無敵シールド被弾音
		{ ResourceLoader::SoundID::BossDamage, boss_damage_se_path },//被弾音
		{ ResourceLoader::SoundID::BossDeath, boss_death_se_path },//死亡音
		{ ResourceLoader::SoundID::BossQuake, boss_quake_se_path },//出現前の地震音

		//浮遊敵・ワームエネミー
		{ ResourceLoader::SoundID::EnemyDeath, enemy_death_se_path },//共通の死亡音
		{ ResourceLoader::SoundID::EnemyShoot, enemy_shoot_se_path },//共通の弾発射音
		{ ResourceLoader::SoundID::EnemyBoot, enemy_boot_se_path },//浮遊敵がactiveになるときの音

		//BGM
		{ ResourceLoader::SoundID::GameBGM, game_bgm_path },//ゲームBGM
		{ ResourceLoader::SoundID::BossBGM, boss_bgm_path },//ボスBGM
		{ ResourceLoader::SoundID::ResultBGM, result_bgm_path },//リザルトBGM
		{ ResourceLoader::SoundID::GameoverBGM, gameover_bgm_path },//ゲームオーバーBGM

		//リザルト
		{ ResourceLoader::SoundID::DataAppear, data_appear_se_path },//カーテン演出音
		{ ResourceLoader::SoundID::ScoreCount, score_count_se_path },//スコア加算音
	};

	//エフェクトの読み込みに必要な情報
	struct EffectInfo
	{
		const wchar_t* path;//パス
		float scale;//拡大率
	};

	//EffectIDと読み込み情報の対応表
	const std::unordered_map<ResourceLoader::EffectID, EffectInfo> effect_infos =
	{
		//プレイヤー
		{ ResourceLoader::EffectID::PlayerBullet, { player_bullet_effect_path, player_bullet_effect_scale } },//弾
		{ ResourceLoader::EffectID::PlayerChargeBullet, { player_charge_bullet_effect_path, player_charge_bullet_effect_scale } },//チャージ弾
		{ ResourceLoader::EffectID::Charging, { charging_effect_path, charging_effect_scale } },//チャージ中
		{ ResourceLoader::EffectID::Boost, { boost_effect_path, boost_effect_scale } },//ブースト
		{ ResourceLoader::EffectID::LeftWingSplash, { left_wing_splash_effect_path, left_wing_splash_effect_scale } },//左羽の水しぶき
		{ ResourceLoader::EffectID::RightWingSplash, { right_wing_splash_effect_path, right_wing_splash_effect_scale } },//右羽の水しぶき
		{ ResourceLoader::EffectID::LeftBarrelRoll, { left_barrel_roll_effect_path, left_barrel_roll_effect_scale } },//左バレルロール
		{ ResourceLoader::EffectID::RightBarrelRoll, { right_barrel_roll_effect_path, right_barrel_roll_effect_scale } },//右バレルロール

		//敵
		{ ResourceLoader::EffectID::EnemyBullet, { enemy_bullet_effect_path, enemy_bullet_effect_scale } },//敵の弾
		{ ResourceLoader::EffectID::WormDeath, { worm_death_effect_path, worm_death_effect_scale } },//ワームの死亡
		{ ResourceLoader::EffectID::FloatingDeath, { floating_death_effect_path, floating_death_effect_scale } },//浮遊敵の死亡

		//ボス
		{ ResourceLoader::EffectID::Splash, { splash_effect_path, splash_effect_scale } },//水しぶき
		{ ResourceLoader::EffectID::SummonFloating, { summon_floating_eff_path, summon_floating_eff_scale } },//浮遊敵召喚
		{ ResourceLoader::EffectID::SummonWorm, { summon_worm_eff_path, summon_worm_eff_scale } },//ワーム召喚
		{ ResourceLoader::EffectID::BossBeam, { boss_beam_eff_patgh, boss_beam_eff_scale } },//ビーム
		{ ResourceLoader::EffectID::BossShield, { boss_shield_effect_path, boss_shield_eff_scale } },//無敵シールド
		{ ResourceLoader::EffectID::BossDeath, { boss_death_eff_path, boss_death_eff_scale } },//死亡

		//共通
		{ ResourceLoader::EffectID::HitEffect, { hit_effect_path, hit_effect_scale } },//被弾(敵味方共通)
	};

	//フォントの読み込みに必要な情報
	struct FontInfo
	{
		const wchar_t* path;//ttfのパス
		LPCWSTR name;//フォント名
		int size;//サイズ
		int thick;//太さ
		int type;//フォントタイプ
		int space;//文字間隔
	};

	//FontIDと読み込み情報の対応表
	const std::unordered_map<ResourceLoader::FontID, FontInfo> font_infos =
	{
		{ ResourceLoader::FontID::Result, { result_font_path, result_font_name, result_font_size, result_font_thick, result_font_type, font_space } },//リザルト用
	};
}

ResourceLoader& ResourceLoader::GetInstance()
{
	//staticでインスタンスを宣言してそれを返す
	static ResourceLoader instance;
	return instance;
}

ResourceLoader::ResourceLoader()
{
	//シーンごとのリソースを先に一覧としてまとめる
	InitSceneResources();
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

void ResourceLoader::LoadGraphic(GraphicID id)
{
	//m_graphicHandlesにidが存在するか確認
	if(m_graphicHandles.find(id) != m_graphicHandles.end())
	{
		//すでに読み込まれている場合は何もしない
		return;
	}

	//読み込み
	auto it = graphic_paths.find(id);
	if(it == graphic_paths.end())
	{
		assert(false && "グラフィックIDが見つかりません");
		return;
	}

	int handle = LoadGraph(it->second);
	assert(handle >= 0 && "グラフィックの読み込みに失敗しました");
	m_graphicHandles[id] = handle;
}

void ResourceLoader::ReleaseAll()
{
	//進行中の非同期読み込みが残っていると
	//解放時にクラッシュするため、完了を待ってから同期に戻す
		WaitHandleASyncLoadAll();
	EndAsyncLoad();

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

void ResourceLoader::ReleaseGraphic(GraphicID id)
{
	auto it = m_graphicHandles.find(id);
	if (it != m_graphicHandles.end())
	{
		DeleteGraph(it->second);
		m_graphicHandles.erase(it);
	}
}

void ResourceLoader::LoadModel(ModelID id)
{
	//m_modelHandlesにidが存在するか確認
	if (m_modelHandles.find(id) != m_modelHandles.end())
	{
		//すでに読み込まれている場合は何もしない
		return;
	}

	//読み込みを行う
	auto it = model_paths.find(id);
	if(it == model_paths.end())
	{
		assert(false && "モデルIDが見つかりません");
		return;
	}

	//モデルを読み込む
	int handle = MV1LoadModel(it->second);
	assert(handle >= 0 && "モデルの読み込みに失敗しました");
	m_modelHandles[id] = handle;//格納
}

void ResourceLoader::ReleaseModel(ModelID id)
{
	auto it = m_modelHandles.find(id);
	if (it != m_modelHandles.end())
	{
		MV1DeleteModel(it->second);
		m_modelHandles.erase(it);
	}
}

void ResourceLoader::LoadEffect(EffectID id)
{
	//m_effectHandlesにidが存在するか確認
	if (m_effectHandles.find(id) != m_effectHandles.end())
	{
		//すでに読み込まれている場合は何もしない
		return;
	}

	auto it = effect_infos.find(id);
	if (it == effect_infos.end())
	{
		assert(false && "エフェクトIDが見つかりません");
		return;
	}
	//エフェクトを読み込む
	int handle = LoadEffekseerEffect(it->second.path, it->second.scale);
	assert(handle >= 0 && "エフェクトの読み込みに失敗しました");
	//ハンドルを登録
	m_effectHandles[id] = handle;
}

void ResourceLoader::ReleaseEffect(EffectID id)
{
	auto it = m_effectHandles.find(id);
	if (it != m_effectHandles.end())
	{
		DeleteEffekseerEffect(it->second);
		m_effectHandles.erase(it);
	}
}

void ResourceLoader::LoadSound(SoundID id)
{
	//m_soundHandlesにidが存在するか確認
	if (m_soundHandles.find(id) != m_soundHandles.end())
	{
		//すでに読み込まれている場合は何もしない
		return;
	}

	//読み込みを行う
	auto it = sound_paths.find(id);
	if(it == sound_paths.end())
	{
		assert(false && "サウンドIDが見つかりません");
		return;
	}

	//サウンドを読み込む
	int handle = LoadSoundMem(it->second);
	assert(handle >= 0 && "サウンドの読み込みに失敗しました");
	m_soundHandles[id] = handle;//格納
}

void ResourceLoader::ReleaseSound(SoundID id)
{
	auto it = m_soundHandles.find(id);
	if (it != m_soundHandles.end())
	{
		DeleteSoundMem(it->second);
		m_soundHandles.erase(it);
	}
}

void ResourceLoader::InitSceneResources()
{
	//シーンが何もない状態はリソースを持たない
	m_sceneResources[SceneID::None] = SceneResources();

	//タイトルのリソース
	SceneResources title;
	title.models = { ModelID::Player };
	title.graphics = {
		GraphicID::TitleLogo,GraphicID::GameStart,GraphicID::GameEnd,
		GraphicID::GameStartOnCursor,GraphicID::GameEndOnCursor,GraphicID::SelectBackGround,
		GraphicID::SkyBoxFront,GraphicID::SkyBoxBack,GraphicID::SkyBoxLeft,
		GraphicID::SkyBoxRight,GraphicID::SkyBoxUp,GraphicID::SkyBoxBottom,
		GraphicID::Caustics, GraphicID::PlayerNormalMap, GraphicID::PlayerMetalicMap,
		GraphicID::PlayerEmissionMap
	};
	title.effects = { EffectID::Boost };
    title.sounds = {
        SoundID::TitleBoost, SoundID::TitleBGM, SoundID::TitleLogoImpact,
        SoundID::OnCursor, SoundID::Decision
    };
    m_sceneResources[SceneID::Title] = title;

    //ゲーム(ポーズ分を含む)
    SceneResources game;
    game.models = {
        ModelID::Player, ModelID::Stage, ModelID::Rock1, ModelID::Rock2,
        ModelID::Rock3, ModelID::FloatingEnemy, ModelID::WormHead, ModelID::Boss
    };
    game.graphics = {
        GraphicID::SkyBoxFront, GraphicID::SkyBoxBack, GraphicID::SkyBoxLeft,
        GraphicID::SkyBoxRight, GraphicID::SkyBoxUp, GraphicID::SkyBoxBottom,
        GraphicID::Caustics, GraphicID::RockNorm, GraphicID::DissolveNoise,
        GraphicID::PlayerNormalMap, GraphicID::PlayerMetalicMap, GraphicID::PlayerEmissionMap,
        GraphicID::EnemyNormalMap, GraphicID::EnemyEmissionMap,
        GraphicID::WormHeadNormalMap, GraphicID::WormHeadMetalicMap, GraphicID::WormHeadEmissionMap,
        GraphicID::WormBodyDiffuseMap, GraphicID::BossNormal, GraphicID::BossEmission,
        GraphicID::NormalReticle, GraphicID::ChargeReticle,
        GraphicID::PlayerHPFrame, GraphicID::PlayerHPGauge,
        GraphicID::BossHPFrame, GraphicID::BossHPGauge,
        GraphicID::SpecialGaugeFrame, GraphicID::SpecialGauge,
        GraphicID::WarningFrame, GraphicID::WarningText, GraphicID::WarningSubText,
        GraphicID::WarningIcon, GraphicID::WarningEdge, GraphicID::BossAppearMovie,
        GraphicID::SelectBackGround, GraphicID::BackGame, GraphicID::BackGameOnCursor,
        GraphicID::BackTitle, GraphicID::BackTitleOnCursor
    };
    game.effects = {
        EffectID::PlayerBullet, EffectID::PlayerChargeBullet, EffectID::Charging,
        EffectID::EnemyBullet, EffectID::HitEffect, EffectID::Boost,
        EffectID::LeftWingSplash, EffectID::RightWingSplash,
        EffectID::LeftBarrelRoll, EffectID::RightBarrelRoll,
        EffectID::FloatingDeath, EffectID::WormDeath,
        EffectID::SummonFloating, EffectID::SummonWorm,
        EffectID::BossBeam, EffectID::BossShield, EffectID::BossDeath, EffectID::Splash
    };
    game.sounds = {
        SoundID::GameBGM, SoundID::BossQuake, SoundID::BossBGM, SoundID::Decision,
        SoundID::PlayerDamage, SoundID::PlayerDeath, SoundID::ChargeShoot,
        SoundID::Somersoult, SoundID::Charging, SoundID::ChargeComplete,
        SoundID::NormalShoot, SoundID::Brake, SoundID::Boost,
        SoundID::BossMove, SoundID::BossDeath, SoundID::BossDamage, SoundID::BossRecovery,
        SoundID::BossBeam, SoundID::BossSummon, SoundID::EnemyShoot,
        SoundID::EnemyBoot, SoundID::EnemyDeath, SoundID::OnCursor
    };
    m_sceneResources[SceneID::Game] = game;

    //クリア
    SceneResources clear;
    clear.graphics = {
        GraphicID::ResultTemplete, GraphicID::SelectBackGround,
        GraphicID::ReTry, GraphicID::ReTryOnCursor,
        GraphicID::BackTitle, GraphicID::BackTitleOnCursor,
        GraphicID::ButtonA, GraphicID::DecideText, GraphicID::NextText
    };
    clear.sounds = {
        SoundID::ResultBGM, SoundID::DataAppear, SoundID::ScoreCount,
        SoundID::Decision, SoundID::OnCursor
    };
    clear.fonts = { FontID::Result };
    m_sceneResources[SceneID::Clear] = clear;

    //ゲームオーバー
    SceneResources gameover;
    gameover.graphics = {
        GraphicID::SelectBackGround, GraphicID::ReTry, GraphicID::ReTryOnCursor,
        GraphicID::GameEnd, GraphicID::GameEndOnCursor,
        GraphicID::ButtonA, GraphicID::DecideText
    };
    gameover.sounds = { SoundID::GameoverBGM, SoundID::OnCursor, SoundID::Decision };
    m_sceneResources[SceneID::Gameover] = gameover;
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

void ResourceLoader::OnSceneChange(SceneID prev, SceneID next)
{
	//テンプレート関数に引数を渡して、リソースを入れ替えてもらう
	const SceneResources& prevResources = m_sceneResources[prev];
	const SceneResources& nextResources = m_sceneResources[next];

	//モデルを入れ替え
	ChangeResources(prevResources.models, nextResources.models,
		&ResourceLoader::LoadModel, &ResourceLoader::ReleaseModel);
	//画像を入れ替え
	ChangeResources(prevResources.graphics, nextResources.graphics,
		&ResourceLoader::LoadGraphic, &ResourceLoader::ReleaseGraphic);
	//エフェクトを入れ替え
	ChangeResources(prevResources.effects, nextResources.effects,
		&ResourceLoader::LoadEffect, &ResourceLoader::ReleaseEffect);
	//音を入れ替え
	ChangeResources(prevResources.sounds, nextResources.sounds,
		&ResourceLoader::LoadSound, &ResourceLoader::ReleaseSound);
	//フォントを入れ替え
	ChangeResources(prevResources.fonts, nextResources.fonts,
		&ResourceLoader::LoadFont, &ResourceLoader::ReleaseFont);
}

void ResourceLoader::BeginAsyncLoad()
{
	//DxLibの非同期ロード開始関数を呼ぶ(TRUEをセットする)
	SetUseASyncLoadFlag(TRUE);
}

void ResourceLoader::EndAsyncLoad()
{
	//DxLibの非同期ロード終了関数を呼ぶ(FALSEをセットする)
	SetUseASyncLoadFlag(FALSE);
}

bool ResourceLoader::IsAsyncLoading() const
{
	//非同期ロード中の数を取得
	//0以上ならまだ非同期ロード中
	return GetASyncLoadNum() > 0;
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

bool ResourceLoader::IsSoundLoaded(SoundID id) const
{
	//サウンドがロードされていればtrueを返す
	return m_soundHandles.find(id) != m_soundHandles.end();
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
	//試しに、対応表をループしてLoadGraphを呼び出す
	for (auto& [id,path] : graphic_paths)
	{
		LoadGraphic(id);
	}
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

	//ブーストエフェクト
	handle = LoadEffekseerEffect(boost_effect_path, boost_effect_scale);
	assert(handle >= 0);
	m_effectHandles[ResourceLoader::EffectID::Boost] = handle;

	//左羽の水しぶきエフェクト
	handle = LoadEffekseerEffect(left_wing_splash_effect_path, left_wing_splash_effect_scale);
	assert(handle >= 0);
	m_effectHandles[ResourceLoader::EffectID::LeftWingSplash] = handle;

	//右羽の水しぶきエフェクト
	handle = LoadEffekseerEffect(right_wing_splash_effect_path, right_wing_splash_effect_scale);
	assert(handle >= 0);
	m_effectHandles[ResourceLoader::EffectID::RightWingSplash] = handle;

	//左バレルロールのエフェクト
	handle = LoadEffekseerEffect(left_barrel_roll_effect_path, left_barrel_roll_effect_scale);
	assert(handle >= 0);
	m_effectHandles[ResourceLoader::EffectID::LeftBarrelRoll] = handle;

	//右バレルロールのエフェクト
	handle = LoadEffekseerEffect(right_barrel_roll_effect_path, right_barrel_roll_effect_scale);
	assert(handle >= 0);
	m_effectHandles[ResourceLoader::EffectID::RightBarrelRoll] = handle;
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

	//プレイヤーの通常ショット
	handle = LoadSoundMem(player_normal_shoot_se_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::NormalShoot] = handle;
	//プレイヤーの死亡
	handle = LoadSoundMem(player_death_se_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::PlayerDeath] = handle;
	//プレイヤーのダメージ
	handle = LoadSoundMem(player_damage_se_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::PlayerDamage] = handle;
	//プレイヤーのチャージショット
	handle = LoadSoundMem(player_charge_shoot_se_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::ChargeShoot] = handle;
	//ブレーキ
	handle = LoadSoundMem(brake_se_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::Brake] = handle;
	//ブースト
	handle = LoadSoundMem(boost_se_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::Boost] = handle;
	//チャージ完了
	handle = LoadSoundMem(charge_complete_se_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::ChargeComplete] = handle;
	//チャージ中
	handle = LoadSoundMem(charging_se_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::Charging] = handle;
	//宙返り
	handle = LoadSoundMem(somersoult_se_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::Somersoult] = handle;

	//ボスの着地音
	handle = LoadSoundMem(boss_move_se_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::BossMove] = handle;
	//ボスのビーム発射音
	handle = LoadSoundMem(boss_beam_se_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::BossBeam] = handle;
	//ボスの雑魚召喚音
	handle = LoadSoundMem(boss_summon_se_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::BossSummon] = handle;
	//ボスの無敵シールド被弾音
	handle = LoadSoundMem(boss_recovery_se_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::BossRecovery] = handle;
	//ボスの被弾音
	handle = LoadSoundMem(boss_damage_se_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::BossDamage] = handle;
	//ボスの死亡音
	handle = LoadSoundMem(boss_death_se_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::BossDeath] = handle;
	//ボス出現前の地震音
	handle = LoadSoundMem(boss_quake_se_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::BossQuake] = handle;

	//浮遊敵・ワームエネミー共通の死亡音
	handle = LoadSoundMem(enemy_death_se_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::EnemyDeath] = handle;
	//浮遊敵・ワームエネミー共通の弾発射音
	handle = LoadSoundMem(enemy_shoot_se_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::EnemyShoot] = handle;
	//浮遊敵がactiveになるときの音
	handle = LoadSoundMem(enemy_boot_se_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::EnemyBoot] = handle;
	//ゲームBGM
	handle = LoadSoundMem(game_bgm_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::GameBGM] = handle;
	//ボスBGM
	handle = LoadSoundMem(boss_bgm_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::BossBGM] = handle;

	//リザルトBGM
	handle = LoadSoundMem(result_bgm_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::ResultBGM] = handle;
	//ゲームオーバーBGM
	handle = LoadSoundMem(gameover_bgm_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::GameoverBGM] = handle;
	//リザルトのカーテン演出音
	handle = LoadSoundMem(data_appear_se_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::DataAppear] = handle;
	//リザルトのスコア加算音
	handle = LoadSoundMem(score_count_se_path);
	assert(handle >= 0);
	m_soundHandles[ResourceLoader::SoundID::ScoreCount] = handle;
}

void ResourceLoader::KeepFont()
{
	//リザルト時のフォント
	LoadFont(FontID::Result);
}

void ResourceLoader::LoadFont(FontID id)
{
	//m_fontHandlesにidが存在するか確認
	if (m_fontHandles.find(id) != m_fontHandles.end())
	{
		//すでに読み込まれている場合は何もしない
		return;
	}

	auto it = font_infos.find(id);
	if (it == font_infos.end())
	{
		assert(false && "フォントIDが見つかりません");
		return;
	}

	//フォントをPC内に一時的に追加
	AddFontResourceEx(it->second.path, FR_PRIVATE, NULL);
	int handle = CreateFontToHandle(
		it->second.name,
		it->second.size,
		it->second.thick,
		it->second.type
	);
	assert(handle >= 0 && "フォントの読み込みに失敗しました");
	SetFontSpaceToHandle(it->second.space, handle);

	//ttfのパスとハンドルを同時に保存
	m_fontHandles[id].handle = handle;
	m_fontHandles[id].path = it->second.path;
}

void ResourceLoader::ReleaseFont(FontID id)
{
	auto it = m_fontHandles.find(id);
	if (it != m_fontHandles.end())
	{
		DeleteFontToHandle(it->second.handle);
		RemoveFontResourceEx(it->second.path, FR_PRIVATE, NULL);
		m_fontHandles.erase(it);
	}
}