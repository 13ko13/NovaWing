#pragma once
#include <DxLib.h>

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
	//Aボタンの画像
	constexpr const wchar_t* a_button_path = L"Data/Image/Button/Button_A.png";
	//決定のテキスト画像
	constexpr const wchar_t* decide_text_path = L"Data/Image/Text/Decide.png";
	//次へのテキスト画像
	constexpr const wchar_t* next_text_path = L"Data/Image/Text/Next.png";
	//リトライ選択肢画像
	constexpr const wchar_t* retry_path = L"Data/Image/SelectFrame/Retry.png";
	//カーソルが乗っているときのリトライ選択肢画像
	constexpr const wchar_t* retry_on_cursor_path = L"Data/Image/SelectFrame/Retry_OnCursor.png";
	//タイトルに戻る選択肢画像
	constexpr const wchar_t* back_title_path = L"Data/Image/SelectFrame/Back_Title.png";
	//カーソルが乗っているときのタイトルに戻る選択肢画像
	constexpr const wchar_t* back_title_on_cursor_path = L"Data/Image/SelectFrame/Back_Title_OnCursor.png";
	//スペシャルゲージの枠画像
	constexpr const wchar_t* special_gauge_frame_path = L"Data/Image/SpecialGauge/Special_Frame.png";
	//スペシャルゲージの画像
	constexpr const wchar_t* special_gauge_path = L"Data/Image/SpecialGauge/Special_Gauge.png";

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

	//----------フォント系-----------------
	//ファイル場所
	constexpr const wchar_t* result_font_path = L"Data/Fonts/Orbitron-Black.ttf";
	//フォント名
	constexpr const wchar_t* result_font_name = L"Orbitron Black";
	//サイズ
	constexpr int result_font_size = 105;
	//太さ
	constexpr int result_font_thick = 5;
	//フォントタイプ
	constexpr int result_font_type = DX_FONTTYPE_ANTIALIASING_4X4;
	//間隔
	constexpr int font_space = 4;

	//----------サウンドのパス--------------
	//タイトルでの飛行音
	constexpr const wchar_t* title_boost_sound_path = L"Data/Sounds/Title/TitleBoost.mp3";
	//決定音
	constexpr const wchar_t* decision_sound_path = L"Data/Sounds/Decision.mp3";
	//カーソルが乗った時の音
	constexpr const wchar_t* on_cursor_sound_path = L"Data/Sounds/OnCursor.mp3";
	//タイトルロゴ出現時の衝撃音
	constexpr const wchar_t* title_logo_impact_sound_path = L"Data/Sounds/Title/TitleLogoImpact.mp3";
	//タイトルBGM
	constexpr const wchar_t* title_bgm_sound_path = L"Data/Sounds/Title/TitleBGM.mp3";
	//プレイヤー通常ショット
	constexpr const wchar_t* player_normal_shoot_se_path = L"Data/Sounds/Game/Player/NormalBullet.mp3";
	//プレイヤー死亡
	constexpr const wchar_t* player_death_se_path = L"Data/Sounds/Game/Player/PlayerDeath.mp3";
	//プレイヤーダメージ
	constexpr const wchar_t* player_damage_se_path = L"Data/Sounds/Game/Player/PlayerDamage.mp3";
	//プレイヤーチャージショット
	constexpr const wchar_t* player_charge_shoot_se_path = L"Data/Sounds/Game/Player/ChargeBullet.mp3";
	//ブレーキ
	constexpr const wchar_t* brake_se_path = L"Data/Sounds/Game/Player/Brake.mp3";
	//ブースト
	constexpr const wchar_t* boost_se_path = L"Data/Sounds/Game/Player/Boost.mp3";
	//チャージ中
	constexpr const wchar_t* charging_se_path = L"Data/Sounds/Game/Player/Charging.mp3";
	//チャージ完了
	constexpr const wchar_t* charge_complete_se_path = L"Data/Sounds/Game/Player/ChargeComplete.mp3";
	//宙返り
	constexpr const wchar_t* somersoult_se_path = L"Data/Sounds/Game/Player/Somersoult2.mp3";

	//ボスの着地音
	constexpr const wchar_t* boss_move_se_path = L"Data/Sounds/Game/Boss/BossMove.mp3";
	//ボスのビーム発射音
	constexpr const wchar_t* boss_beam_se_path = L"Data/Sounds/Game/Boss/BossBeam.mp3";
	//ボスの雑魚召喚音
	constexpr const wchar_t* boss_summon_se_path = L"Data/Sounds/Game/Boss/Summon.mp3";
	//ボスの無敵シールド被弾音
	constexpr const wchar_t* boss_recovery_se_path = L"Data/Sounds/Game/Boss/Recovery.mp3";
	//ボスの被弾音
	constexpr const wchar_t* boss_damage_se_path = L"Data/Sounds/Game/Boss/BossDamage.mp3";
	//ボスの死亡音
	constexpr const wchar_t* boss_death_se_path = L"Data/Sounds/Game/Boss/BossDeath.mp3";
	//ボス出現前の地震音
	constexpr const wchar_t* boss_quake_se_path = L"Data/Sounds/Game/Boss/Quake.mp3";

	//浮遊敵・ワームエネミー共通の死亡音(爆発音)
	constexpr const wchar_t* enemy_death_se_path = L"Data/Sounds/Game/EnemyDeath.mp3";
	//浮遊敵・ワームエネミー共通の弾発射音
	constexpr const wchar_t* enemy_shoot_se_path = L"Data/Sounds/Game/EnemyShoot.mp3";
	//浮遊敵がactiveになるときの音
	constexpr const wchar_t* enemy_boot_se_path = L"Data/Sounds/Game/Floating/EnemyBoot.mp3";
	//ワームエネミーの移動音
	constexpr const wchar_t* worm_move_se_path = L"Data/Sounds/Game/Worm/WormMove.mp3";

	//ゲームBGM
	constexpr const wchar_t* game_bgm_path = L"Data/Sounds/Game/GameBGM.mp3";
	//ボスBGM
	constexpr const wchar_t* boss_bgm_path = L"Data/Sounds/Game/BossBGM.mp3";

	//リザルトBGM
	constexpr const wchar_t* result_bgm_path = L"Data/Sounds/Result/ResultBGM.mp3";
	//リザルトのカーテン演出音
	constexpr const wchar_t* data_appear_se_path = L"Data/Sounds/Result/DataAppear.mp3";
	//リザルトのスコア加算音
	constexpr const wchar_t* score_count_se_path = L"Data/Sounds/Result/Score.mp3";
	//ゲームオーバーBGM
	constexpr const wchar_t* gameover_bgm_path = L"Data/Sounds/Result/GameoverBGM.mp3";
}