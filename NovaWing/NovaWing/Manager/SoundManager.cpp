#define NOMINMAX
#include "SoundManager.h"
#include <DxLib.h>
#include <algorithm>

namespace
{
	//タイトルでのブーストの音量
	constexpr int title_boost_volume = 150;
	//カーソルが乗った時の音の音量
	constexpr int on_cursor_volume = 150;
	//タイトルロゴ出現時の衝撃音の音量
	constexpr int title_impact_volume = 150;
	//タイトルBGMの音量
	constexpr int title_bgm_volume = 150;
	//プレイヤー通常弾
	constexpr int normal_shoot_volume = 150;
	//プレイヤー死亡
	constexpr int player_death_volume = 150;
	//プレイヤーダメージ
	constexpr int player_damage_volume = 150;
	//プレイヤーチャージショット
	constexpr int player_charge_shoot_volume = 180;
	//ブレーキ
	constexpr int brake_volume = 125;
	//ブースト
	constexpr int boost_volume = 125;
	//チャージ完了
	constexpr int charge_complete_volume = 150;
	//チャージ中
	constexpr int charging_volume = 150;
	//宙返り
	constexpr int somersoult_volume = 150;

	//ボスの着地音
	constexpr int boss_move_volume = 180;
	//ボスのビーム発射音
	constexpr int boss_beam_volume = 180;
	//ボスの雑魚召喚音
	constexpr int boss_summon_volume = 150;
	//ボスの無敵シールド被弾音
	constexpr int boss_recovery_volume = 125;
	//ボスの被弾音
	constexpr int boss_damage_volume = 150;
	//ボスの死亡音
	constexpr int boss_death_volume = 180;
	//ボス出現前の地震音
	constexpr int boss_quake_volume = 160;

	//浮遊敵・ワームエネミー共通の死亡音
	constexpr int enemy_death_volume = 125;
	//浮遊敵・ワームエネミー共通の弾発射音
	constexpr int enemy_shoot_volume = 70;
	//浮遊敵がactiveになるときの音
	constexpr int enemy_boot_volume = 125;
	//ワームエネミーの移動音
	constexpr int worm_move_volume = 100;

	//ゲームBGM
	constexpr int game_bgm_volume = 150;
	//ボスBGM
	constexpr int boss_bgm_volume = 150;
	//リザルトBGM
	constexpr int result_bgm_volume = 150;
	//リザルトのカーテン演出音
	constexpr int data_appear_volume = 125;
	//決定音
	constexpr int decision_volume = 190;
	//リザルトのスコア加算音
	constexpr int score_count_volume = 190;
}

SoundManager::SoundManager()
{

}

SoundManager::~SoundManager()
{
	//全ての音を停止する
	StopAll();
}

void SoundManager::Init()
{
	//リソースローダーのインスタンスを取得
	auto& loader = ResourceLoader::GetInstance();

	//それぞれのハンドルを取得して設定する
	//タイトルでのプレイヤーのブースト音
	InitData(SoundType::TitleBoost,
		ResourceLoader::SoundID::TitleBoost,
		true, title_boost_volume, false);
	//カーソルが乗った時の音
	InitData(SoundType::OnCursor,
		ResourceLoader::SoundID::OnCursor,
		true, on_cursor_volume, false);
	//決定音
	InitData(SoundType::Decision,
		ResourceLoader::SoundID::Decision,
		true, decision_volume, false);
	//タイトルロゴ出現時の衝撃音
	InitData(SoundType::TitleLogoImpact,
	ResourceLoader::SoundID::TitleLogoImpact,
	true, title_impact_volume, false);
	//タイトルBGM
	InitData(SoundType::TitleBGM,
	ResourceLoader::SoundID::TitleBGM,
	true, title_bgm_volume, true);

	//プレイヤー通常弾
	InitData(SoundType::NormalShoot,
	ResourceLoader::SoundID::NormalShoot,
	true, normal_shoot_volume, false);
	//プレイヤー死亡
	InitData(SoundType::PlayerDeath,
	ResourceLoader::SoundID::PlayerDeath,
	true, player_death_volume, false);
	//プレイヤーダメージ
	InitData(SoundType::PlayerDamage,
	ResourceLoader::SoundID::PlayerDamage,
	true, player_damage_volume, false);
	//チャージショット
	InitData(SoundType::ChargeShoot,
	ResourceLoader::SoundID::ChargeShoot,
	true, player_charge_shoot_volume, false);
	//ブレーキ
	InitData(SoundType::Brake,
	ResourceLoader::SoundID::Brake,
	true, brake_volume, false);
	//ブースト
	InitData(SoundType::Boost,
		ResourceLoader::SoundID::Boost,
		true, boost_volume, false);
	//チャージ完了
	InitData(SoundType::ChargeComplete,
		ResourceLoader::SoundID::ChargeComplete,
		true, charge_complete_volume, false);
	//チャージ中
	InitData(SoundType::Charging,
		ResourceLoader::SoundID::Charging,
		true, charging_volume, true);
	//宙返り
	InitData(SoundType::Somersoult,
		ResourceLoader::SoundID::Somersoult,
		true, somersoult_volume, false);

	//ボスの着地音
	InitData(SoundType::BossMove,
		ResourceLoader::SoundID::BossMove,
		true, boss_move_volume, false);
	//ボスのビーム発射音
	InitData(SoundType::BossBeam,
		ResourceLoader::SoundID::BossBeam,
		true, boss_beam_volume, false);
	//ボスの雑魚召喚音
	InitData(SoundType::BossSummon,
		ResourceLoader::SoundID::BossSummon,
		true, boss_summon_volume, false);
	//ボスの無敵シールド被弾音
	InitData(SoundType::BossRecovery,
		ResourceLoader::SoundID::BossRecovery,
		true, boss_recovery_volume, false);
	//ボスの被弾音
	InitData(SoundType::BossDamage,
		ResourceLoader::SoundID::BossDamage,
		true, boss_damage_volume, false);
	//ボスの死亡音
	InitData(SoundType::BossDeath,
		ResourceLoader::SoundID::BossDeath,
		true, boss_death_volume, false);
	//ボス出現前の地震音
	InitData(SoundType::BossQuake,
		ResourceLoader::SoundID::BossQuake,
		true, boss_quake_volume, false);

	//浮遊敵・ワームエネミー共通の死亡音
	InitData(SoundType::EnemyDeath,
		ResourceLoader::SoundID::EnemyDeath,
		true, enemy_death_volume, false);
	//浮遊敵・ワームエネミー共通の弾発射音
	InitData(SoundType::EnemyShoot,
		ResourceLoader::SoundID::EnemyShoot,
		true, enemy_shoot_volume, false);
	//浮遊敵がactiveになるときの音
	InitData(SoundType::EnemyBoot,
		ResourceLoader::SoundID::EnemyBoot,
		true, enemy_boot_volume, false);
	//ワームエネミーの移動音(ループ)
	InitData(SoundType::WormMove,
		ResourceLoader::SoundID::WormMove,
		true, worm_move_volume, true);

	//ゲームBGM(ループ)
	InitData(SoundType::GameBGM,
		ResourceLoader::SoundID::GameBGM,
		true, game_bgm_volume, true);
	//ボスBGM(ループ)
	InitData(SoundType::BossBGM,
		ResourceLoader::SoundID::BossBGM,
		true, boss_bgm_volume, true);
	//リザルトBGM(ループ)
	InitData(SoundType::ResultBGM,
		ResourceLoader::SoundID::ResultBGM,
		true, result_bgm_volume, true);
	//リザルトのカーテン演出音
	InitData(SoundType::DataAppear,
		ResourceLoader::SoundID::DataAppear,
		true, data_appear_volume, false);
	//リザルトのスコア加算音
	InitData(SoundType::ScoreCount,
		ResourceLoader::SoundID::ScoreCount,
		true, score_count_volume, false);
}

void SoundManager::Update()
{
	for (auto& [type, data] : m_sounds)
	{
		if (data.fadeState == FadeState::None) continue;//フェードしてないならスキップ

		//フェードの経過時間を更新する
		data.fadeTimer++;

		if (data.fadeState == FadeState::FadeIn)
		{
			//targetVolumeに向かってcurrentVolumeを増やす
			//fadeの進行度割合に応じてcurrentVolumeを更新する
			data.currentVolume = static_cast<int>(data.fadeTimer / data.fadeDuration * data.targetVolume);
			//音量が255を超えないようにする
			data.currentVolume = std::min(data.currentVolume, data.targetVolume);

			//currentVolumeがtargetVolumeに達したらフェードイン完了
			if (data.currentVolume >= data.targetVolume)
			{
				data.currentVolume = data.targetVolume;//currentVolumeをtargetVolumeに合わせる
				//音量を変更する
				data.fadeState = FadeState::None;//フェード状態をNoneにする
			}
			ChangeVolumeSoundMem(data.currentVolume, data.handle);
		}
		else if (data.fadeState == FadeState::FadeOut)
		{
			//targetVolumeに向かってcurrentVolumeを減らす
			data.currentVolume = static_cast<int>(data.volume * (1.0f - data.fadeTimer / data.fadeDuration));
			//音量を変更する
			ChangeVolumeSoundMem(data.currentVolume, data.handle);
			//currentVolumeが0になったらフェードアウト完了
			if (data.currentVolume <= 0)
			{
				data.currentVolume = 0;//currentVolumeを0に合わせる
				//音を停止する
				StopSoundMem(data.handle);
				data.fadeState = FadeState::None;//フェード状態をNoneにする
			}
		}
	}
}

void SoundManager::PlayFadeIn(SoundType soundType, float duration, bool loop)
{
	//ロードされていなかったら以降の処理をスキップ
	auto it = m_sounds.find(soundType);
	if (it == m_sounds.end() || !it->second.loaded) return;

	//サウンドのデータを取得する
	auto& data = it->second;

	//フェードインの設定をする
	data.fadeState = FadeState::FadeIn;//フェード状態をフェードインにする
	data.currentVolume = 0;//現在の音量を0にする
	data.targetVolume = data.volume;//目標の音量をデータの音量にする
	data.fadeTimer = 0.0f;//フェードの経過時間を0にする
	data.fadeDuration = duration;//フェードにかける時間を設定する

	//音を再生する
	ChangeVolumeSoundMem(0, data.handle);//音量を0にする
	PlaySoundMem(
		data.handle,
		loop ? DX_PLAYTYPE_LOOP : DX_PLAYTYPE_BACK,//loopがtrueならループそうじゃないならバック再生
		true);
}

void SoundManager::FadeOut(SoundType soundType, float duration)
{
	//ロードされていなかったら以降の処理をスキップ
	auto it = m_sounds.find(soundType);
	if (it == m_sounds.end() || !it->second.loaded) return;

	//サウンドのデータを取得する
	auto& data = it->second;

	//フェードアウトの設定をする
	data.fadeState = FadeState::FadeOut;//フェード状態をフェードアウトにする
	data.currentVolume = data.volume;//現在の音量をデータの音量にする
	data.targetVolume = 0;//目標の音量を0にする
	data.fadeTimer = 0.0f;//フェードの経過時間を0にする
	data.fadeDuration = duration;//フェードにかける時間を設定する
}

void SoundManager::Play(SoundType soundType, bool loop, bool isOnce)
{
	//ロードされていなかったら以降の処理をスキップ
	auto it = m_sounds.find(soundType);
	if (it == m_sounds.end() || !it->second.loaded) return;

	auto& data = it->second;
	//フェード状態をリセットする
	data.fadeState = FadeState::None;
	//音量をデフォルトの音量に戻す
	ChangeVolumeSoundMem(data.volume, data.handle);

	//既に再生中なら再生しない
	if (data.isLoop && CheckSoundMem(data.handle) == 1) return;

	//isOnce指定時は、既に再生中なら重ねて鳴らさない
	if (isOnce && CheckSoundMem(data.handle) == 1) return;

	PlaySoundMem(
		data.handle,
		loop ? DX_PLAYTYPE_LOOP : DX_PLAYTYPE_BACK,
		true);
}

void SoundManager::Stop(SoundType type)
{
	auto it = m_sounds.find(type);
	if (it == m_sounds.end() || !it->second.loaded) return;

	//指定した音を停止させる
	StopSoundMem(it->second.handle);
}

void SoundManager::StopAll()
{
	for (auto& sound : m_sounds)
	{
		//すべての音を停止する
		StopSoundMem(sound.second.handle);
	}
}

void SoundManager::SetVolume(SoundType type, int volume)
{
	auto it = m_sounds.find(type);
	if (it == m_sounds.end()) return;

	auto& data = it->second;
	//音量を矯正
	data.volume = std::clamp(volume, 0, 255);

	//ちゃんとロードされてたら音量を変更する
	if (data.loaded && data.handle != -1)
	{
		ChangeVolumeSoundMem(data.volume, data.handle);
	}
}

void SoundManager::InitData(
	SoundType type, 
	ResourceLoader::SoundID soundID,
	bool isLoaded, int volume, bool isLoop)
{
	//リソースローダーのインスタンスを取得
	auto& loader = ResourceLoader::GetInstance();

	//それぞれのハンドルを取得して設定する
	//タイトルでのプレイヤーのブースト音
	auto& soundData = m_sounds[type];
	soundData.handle = loader.GetSound(soundID);
	soundData.loaded = isLoaded;//ロード済みフラグを立てる
	soundData.volume = volume;
	soundData.isLoop = isLoop;//ループしない
	ChangeVolumeSoundMem(soundData.volume, soundData.handle);//音量を変更する
}
