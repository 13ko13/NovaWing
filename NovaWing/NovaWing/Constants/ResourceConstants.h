#pragma once

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
	//プレイヤーHPゲージ
	constexpr const wchar_t* player_hp_frame_path = L"Data/Image/HPGauge/PlayerHPGauge/HP_Frame_fix.png";
	constexpr const wchar_t* player_hp_gauge_path = L"Data/Image/HPGauge/PlayerHPGauge/HP_Gauge_fix.png";
	//ボス
	constexpr const wchar_t* boss_normal_path = L"Data/Model/Boss.fbm/T_Mech_LOD2_N.png";
	constexpr const wchar_t* boss_emission_path = L"Data/Model/Boss.fbm/T_Mech_LOD2_E.png";
	//ボスHPゲージ
	constexpr const wchar_t* boss_hp_frame_path = L"Data/Image/HPGauge/BossHPGauge/Boss_HP_Frame.png";
	constexpr const wchar_t* boss_hp_gauge_path = L"Data/Image/HPGauge/BossHPGauge/Boss_HP_Ber.png";
	//リザルトのテンプレート
	constexpr const wchar_t* result_templete_path = L"Data/Image/Result_Templete.png";

	//---------- エフェクトのパス ----------
	constexpr const wchar_t* player_bullet_effect_path = L"Data/Effect/PlayerBullet/PlayerBullet.efk";
	constexpr const wchar_t* worm_death_effect_path = L"Data/Effect/Exprosion/Exprosion.efk";
	constexpr const wchar_t* floating_death_effect_path = L"Data/Effect/Exprosion2/Exprosion2.efk";
	constexpr const wchar_t* player_charge_bullet_effect_path = L"Data/Effect/PlayerChargeBullet/PlayerChargeBullet.efk";
	constexpr const wchar_t* charging_effect_path = L"Data/Effect/Charging/Charging.efk";
	constexpr const wchar_t* enemy_bullet_effect_path = L"Data/Effect/EnemyBullet/EnemyBullet.efk";
	//水しぶき
	constexpr const wchar_t* splash_effect_path = L"Data/Effect/Splash/Splash.efk";
	//浮遊敵召喚
	constexpr const wchar_t* summon_floating_eff_path = L"Data/Effect/SummonFloatingEnemy/SummonFloating.efk";
	//ワーム召喚
	constexpr const wchar_t* summon_worm_eff_path = L"Data/Effect/SummonWormEnemy/SummonWorm.efk";
	//ボスのビーム
	constexpr const wchar_t* boss_beam_eff_patgh = L"Data/Effect/BossBeam/BossBeam.efk";
	//ボスの無敵エフェクト
	constexpr const wchar_t* boss_shield_effect_path = L"Data/Effect/BossShield/BossShield.efk";
	//ヒットエフェクト(敵味方共通)
	constexpr const wchar_t* hit_effect_path = L"Data/Effect/HitEffect/HitEffect.efk";
	//ボスの死亡エフェクト
	constexpr const wchar_t* boss_death_eff_path = L"Data/Effect/BossDeath/BossDeath.efk";

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
	constexpr float summon_floating_eff_scale = 1.0f;
	constexpr float summon_worm_eff_scale = 1.0f;
	constexpr float boss_beam_eff_scale = 1.0f;
	constexpr float boss_shield_eff_scale = 4.0f;
	constexpr float hit_effect_scale = 2.0f;
	constexpr float boss_death_eff_scale = 1.0f;
}