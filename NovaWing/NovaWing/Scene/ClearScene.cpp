#include <string>
#include <algorithm>

#include "ClearScene.h"
#include "GameScene.h"
#include "Manager/InputManager.h"
#include "Scene/TitleScene.h"
#include "SceneController.h"
#include "Main/Application.h"
#include "Utility/GraphShaderDraw.h"
#include "Constants/ShaderRegister.h"
#include "Manager/SoundManager.h"

namespace
{
	//決定キーの遷移先に切り替わるまでのフレーム数
	constexpr float scene_change_frame = 60.0f;

	//選択肢のカーソル
	constexpr const wchar_t* cursor_text = L"→";
	//クリア表示のテキスト
	constexpr const wchar_t* game_clear_text = L"GAME CLEAR";

	//選択肢のテキスト
	constexpr const wchar_t* back_title_text = L"タイトルに戻る";
	constexpr const wchar_t* exit_game_text = L"ゲーム終了";

	//シェーダにフレームを渡すときに値が大きすぎるので小さくするための値
	constexpr float time_speed = 0.1f;
	//スキャンラインを入れる周期
	constexpr float scanline_frequency = 280.0f;

	//テンプレート画像の開く演出を何フレームかけるか
	constexpr int templete_opne_max_frame = 30;
	//選択肢の背景画像の開く演出を何フレームかけるか
	constexpr int background_opne_max_frame = 30;

	//テンプレート画像のサイズ
	constexpr float templete_size = 1.125f;

	//敵を倒した数を表示する座標
	const Vector2 kill_count_pos = Vector2(0.7031f, 0.2778f);
	//クリアタイムを表示する座標
	const Vector2 clear_time_pos = Vector2(0.7031f, 0.4167f);
	//被弾回数を表示する座標
	const Vector2 hit_count_pos = Vector2(0.7031f, 0.5694f);
	//スコアを表示する座標
	const Vector2 score_pos = Vector2(0.7031f, 0.7458f);

	//文字をぼかす範囲
	constexpr int blur_range = 16;
	//ぼかしの強さ
	constexpr int blur_strength = 1400;

	//ぼかしの透明度
	constexpr int blur_alpha = 255;

	//数字をLerpするときの速度
	constexpr float count_lerp_speed = 0.1f;
	constexpr float score_count_lerp_speed = 0.05f;

	//lerpされた数字を押し戻して丸めるときの閾値
	constexpr float lerp_guard_threshould = 0.5f;

	//スコアとして計算するときの倍率
	constexpr int score_multiplier = 100;
	//キルのみ倍率をあげる
	constexpr int kill_score_multiplier = 5000;

	//キル数、クリアタイム、スコアを描画する色
	constexpr int score_color = 0x00ffaa;

	//被弾回数を描画する色
	constexpr int hit_count_color = 0xe13c3c;

	//Aボタン画像の描画座標
	const Vector2 a_button_pos = Vector2(0.8984f, 0.9583f);
	//つぎへのテキスト画像の描画座標
	const Vector2 next_graph_pos = Vector2(0.9297f, 0.9583f);
	//決定のテキスト画像の描画座標
	const Vector2 decide_graph_pos = Vector2(0.9297f, 0.9583f);
	//Aボタン画像のサイズ
	constexpr double a_button_scale = 0.45;
	//つぎへのテキスト画像のサイズ
	constexpr double next_graph_scale = 0.45;
	//決定のテキスト画像のサイズ
	constexpr double decide_graph_scale = 0.45;

	//選択肢画像のサイズ
	constexpr double select_graph_scale = 1.2;
	//選択肢の背景画像のサイズ
	constexpr double background_graph_scale = 2.25;

	//リトライ選択肢の位置
	const Vector2 retry_ratio = Vector2(0.5f, 0.43f);//画面に対してどのあたりにしたいか
	//タイトルに戻る選択肢の位置
	const Vector2 back_title_ratio = Vector2(0.5f, 0.63f);//画面に対してどのあたりにしたいか

	//カーソルが乗った時に何秒で画像を切り替えるか
	constexpr int wipe_max = 15;

	//スコア加算音を鳴らせる間隔
	constexpr int score_count_sound_interval = 5;
}

ClearScene::ClearScene(SceneController& controller, const ClearResultData& data) :
	Scene(controller),
	m_resultData(data)
{
}

ClearScene::~ClearScene()
{
	//オフスクリーン画像を開放
	DeleteGraph(m_textRenderTargetH);
	//オフスクリーン画像を開放
	DeleteGraph(m_textGlowH);
}

void ClearScene::Init()
{
	//サウンドマネージャーの初期化
	m_pSoundManager = std::make_shared<SoundManager>();
	m_pSoundManager->Init();

	//リザルトBGMを鳴らす
	m_pSoundManager->Play(SoundManager::SoundType::ResultBGM, true);

	//グリッチシェーダのロード
	m_glitchPSH = LoadPixelShader(L"GlitchPS.pso");

	//シェーダバッファを作成
	m_cbufferGlitch = CreateShaderConstantBuffer(sizeof(GlitchBuffer));
	m_pCBuffGlitchData = static_cast<GlitchBuffer*>(GetBufferShaderConstantBuffer(m_cbufferGlitch));
	//スキャンラインを入れる周期をシェーダに渡す
	m_pCBuffGlitchData->scanlineFrequency = scanline_frequency; //適切な値を決める必要あり
	UpdateShaderConstantBuffer(m_cbufferGlitch);

	//スコアを先に計算しておく
	m_score = m_resultData.defeatedEnemyCount * kill_score_multiplier -
		(m_resultData.clearTime / 60 + m_resultData.hitCount) * score_multiplier;

	//リザルト情報を描画
	int fontHandle = ResourceLoader::GetInstance().GetFont(
		ResourceLoader::FontID::Result);
	//ウィンドウサイズ
	Size wsize = Application::GetInstance().GetWindowSize();

	//文字にもシェーダをかけるために1枚の画像として
	//文字のみ描画された画像を作るためにオフスクリーンを用意
	m_textRenderTargetH = MakeScreen(wsize.m_width, wsize.m_height, true);
	//発光用のぼかし画像を作るためにオフスクリーンを用意
	m_textGlowH = MakeScreen(wsize.m_width, wsize.m_height, true);
	//リザルト情報をオフスクリーンに描画描画(シェーダ適用済み+ぼかし適用済み)
	DrawResultText(fontHandle);
}

void ClearScene::Update()
{
	//InputManagerのインスタンスを取得
	InputManager& input = InputManager::GetInstance();
	//フレーム更新
	m_frame++;
	m_pCBuffGlitchData->time = m_frame * time_speed;//シェーダ用のフレームに変換して渡す
	UpdateShaderConstantBuffer(m_cbufferGlitch);

	//サウンドマネージャーの更新
	m_pSoundManager->Update();

	//テンプレートの開く演出用のフレーム更新
	if (!m_controller.GetFade().IsFading() &&
		!m_isPushNextButton)
	{
		//開き始めた瞬間にカーテン演出音を鳴らす
		if (m_templeteOpenFrame == 0)
		{
			m_pSoundManager->Play(SoundManager::SoundType::DataAppear);
		}

		m_templeteOpenFrame++;
		if (m_templeteOpenFrame > templete_opne_max_frame)
		{
			m_templeteOpenFrame = templete_opne_max_frame;
		}
	}

	//テンプレートの開く演出が終わったら数字をLerpで近づける
	if (m_templeteOpenFrame >= templete_opne_max_frame)
	{
		//キル数
		m_currentKillCount = std::lerp(
			m_currentKillCount,
			static_cast<float>(m_resultData.defeatedEnemyCount),
			count_lerp_speed
		);
		//クリアタイム
		m_currentClearTime = std::lerp(
			m_currentClearTime,
			static_cast<float>(m_resultData.clearTime / 60),
			count_lerp_speed
		);
		//ダメージ数
		m_currentHitCount = std::lerp(
			m_currentHitCount,
			static_cast<float>(-m_resultData.hitCount),
			count_lerp_speed
		);
		//スコア
		m_currentScore = std::lerp(
			m_currentScore,
			static_cast<float>(m_score),
			score_count_lerp_speed
		);

		//lerpはぴったり止まらない可能性があるので、
		//目標値との差が近くなったら目標値に丸める
		//キル数
		if (std::abs(m_currentKillCount -
			static_cast<float>(m_resultData.defeatedEnemyCount)) < lerp_guard_threshould)
		{
			m_currentKillCount = static_cast<float>(m_resultData.defeatedEnemyCount);
		}
		//クリアタイム
		if (std::abs(m_currentClearTime -
			static_cast<float>(m_resultData.clearTime / 60)) < lerp_guard_threshould)
		{
			m_currentClearTime = static_cast<float>(m_resultData.clearTime / 60);
		}
		//被弾回数
		if (std::abs(m_currentHitCount -
			static_cast<float>(-m_resultData.hitCount)) < lerp_guard_threshould)
		{
			m_currentHitCount = static_cast<float>(-m_resultData.hitCount);
		}
		//スコア
		if (std::abs(m_currentScore -
			static_cast<float>(m_score)) < lerp_guard_threshould)
		{
			m_currentScore = static_cast<float>(m_score);
		}

		//4つの項目が全てlerp終了しているかを判定
		if (m_currentKillCount == m_resultData.defeatedEnemyCount &&
			m_currentClearTime == m_resultData.clearTime / 60 &&
			m_currentHitCount == -m_resultData.hitCount &&
			m_currentScore == m_score)
		{
			//終っていたらフラグを立てる
			m_isLerpFinished = true;
		}

		//lerpがまだ終わっていない間、クールタイムを設けてスコア加算音を鳴らす
		if (!m_isLerpFinished)
		{
			m_scoreCountSoundCT++;
			if (m_scoreCountSoundCT >= score_count_sound_interval)
			{
				m_pSoundManager->Play(SoundManager::SoundType::ScoreCount);
				m_scoreCountSoundCT = 0;
			}
		}

		//新しくlerpによって完成した値をもとにオフスクリーンに描画しておく
		//リザルト情報を描画
		int fontHandle = ResourceLoader::GetInstance().GetFont(
			ResourceLoader::FontID::Result);
		DrawResultText(fontHandle);
	}

	//lerpが終了しているときに、nextボタンが押されたら
	if (m_isLerpFinished)
	{
		//選択系をカーテン演出で出すようにする
		if (input.IsTriggered(InputEvent::next))
		{
			//次へボタンが押されたという情報を持つ
			m_isPushNextButton = true;

			//次へボタンの音を鳴らす
			m_pSoundManager->Play(SoundManager::SoundType::Decision);
		}
		//次へボタンが押されていたら
		if (m_isPushNextButton)
		{
			m_templeteOpenFrame--;
			if (m_templeteOpenFrame < 0)
			{
				m_templeteOpenFrame = 0;
				//閉じる演出が終ってから開くようにする
				m_backGroundOpenFrame++;
			}

			//クランプ
			if (m_backGroundOpenFrame > background_opne_max_frame)
			{
				m_backGroundOpenFrame = background_opne_max_frame;
			}

			//下入力で選択肢を下に移動(indexを増やす)
			if (input.IsTriggered(InputEvent::down))
			{
				//選択肢の最大数で割った余りを取ることで、
				//選択肢の範囲内に収める
				m_selectIndex =
					static_cast<ClearSelect>(
						(static_cast<int>(m_selectIndex) + 1) %
						static_cast<int>(ClearSelect::SelectMax));
				//選択音を鳴らす
				m_pSoundManager->Play(SoundManager::SoundType::OnCursor);
			}
			//上入力で選択肢を上に移動(indexを減らす)
			if (input.IsTriggered(InputEvent::up))
			{
				//選択肢の最大数で割った余りを取ることで、
				//選択肢の範囲内に収める
				m_selectIndex =
					static_cast<ClearSelect>(
						(static_cast<int>(m_selectIndex) - 1 + static_cast<int>(ClearSelect::SelectMax)) %
						static_cast<int>(ClearSelect::SelectMax));
				//選択音を鳴らす
				m_pSoundManager->Play(SoundManager::SoundType::OnCursor);
			}
			//決定入力で選択肢を決定する
			//背景が開ききっているときだけ決定入力を受け付ける
			if (m_backGroundOpenFrame >= background_opne_max_frame &&
				input.IsTriggered(InputEvent::ok))
			{
				//決定音を鳴らす
				m_pSoundManager->Play(SoundManager::SoundType::Decision);

				switch (m_selectIndex)
				{
				case ClearSelect::ReTry:
				{
					//ゲームシーンに遷移する
					m_controller.ChangeScene(
						std::make_shared<GameScene>(
							m_controller), scene_change_frame);
					break;
				}
				case ClearSelect::BackTitle:
				{
					//タイトルシーンに遷移する
					m_controller.ChangeScene(
						std::make_shared<TitleScene>(
							m_controller), scene_change_frame);
					break;
				}
				}
			}

			//現在の選択肢と前のフレームの選択肢を比較して
			//変わっていたらワイプの進行度をリセット
			if (m_selectIndex != m_prevSelectIdx)
			{
				m_wipeProgress[static_cast<int>(m_selectIndex)] = 0.0f;
			}
			//選択肢が一致しているワイプ進行度を増やす
			//一致しないもののワイプ進行度を減らす
			for (int i = 0; i < static_cast<int>(ClearSelect::SelectMax); i++)
			{
				//今回見たい選択肢
				ClearSelect current = static_cast<ClearSelect>(i);//iをTitleSelectの番号としてみる
				//選択肢が一致している場合
				if (current == m_selectIndex)
				{
					m_wipeProgress[i] += 1.0f / wipe_max;

				}
				else//一致しない場合
				{
					m_wipeProgress[i] -= 1.0f / wipe_max;
				}
				m_wipeProgress[i] = std::clamp(m_wipeProgress[i], 0.0f, 1.0f);//0~1にクランプ
			}

			//前フレームの選択肢を保存
			m_prevSelectIdx = m_selectIndex;
		}
	}
}

void ClearScene::Draw()
{
	//ウィンドウサイズ
	Size wsize = Application::GetInstance().GetWindowSize();
	//画面の真ん中
	int x = wsize.m_width / 2;
	int y = wsize.m_height / 2;
	ResourceLoader& loader = ResourceLoader::GetInstance();

	SetUsePixelShader(m_glitchPSH);
	SetShaderConstantBuffer(m_cbufferGlitch, DX_SHADERTYPE_PIXEL, ShaderRegister::glitch_buffer);

	//画像をカーテンのように開く感じで表示するために
	//進行度計算
	float openProgress = static_cast<float>(m_templeteOpenFrame) /
		static_cast<float>(templete_opne_max_frame);

	//中心を基準に左右対称の範囲を計算
	float uvMinU = 0.5f - openProgress * 0.5f;
	float uvMaxU = 0.5f + openProgress * 0.5f;

	//リザルト用のテンプレート画像配置
	int handle = ResourceLoader::GetInstance().GetGraphic(
		ResourceLoader::GraphicID::ResultTemplete);
	DrawGraphToShaderByCenter(
		wsize.m_width * 0.5f,
		wsize.m_height * 0.5f,
		templete_size,
		handle,
		1.0f,
		uvMaxU,
		uvMinU
	);

	//オフスクリーンに描画しておいた、ぼかし画像を描画
	//ぼかし画像を先に加算合成で描画する
	SetDrawBlendMode(DX_BLENDMODE_ADD, blur_alpha);
	DrawGraphToShaderByCenter(
		wsize.m_width * 0.5f, wsize.m_height * 0.5f,
		1.0, m_textGlowH,
		1.0f, uvMaxU, uvMinU
	);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 255);

	DrawGraphToShaderByCenter(
		wsize.m_width * 0.5f, wsize.m_height * 0.5f,
		1.0, m_textRenderTargetH,
		1.0f, uvMaxU, uvMinU
	);

	if (m_isPushNextButton)
	{
		//画像をカーテンのように開く感じで表示するために
		//進行度計算
		openProgress = static_cast<float>(m_backGroundOpenFrame) /
			static_cast<float>(background_opne_max_frame);

		//中心を基準に左右対称の範囲を計算
		uvMinU = 0.5f - openProgress * 0.5f;
		uvMaxU = 0.5f + openProgress * 0.5f;

		//選択肢背景画像
		int backgroundH = loader.GetGraphic(ResourceLoader::GraphicID::SelectBackGround);
		DrawGraphToShaderByCenter(
			wsize.m_width * 0.5f, wsize.m_height * 0.5f,
			background_graph_scale, backgroundH,
			1.0f,
			uvMaxU,
			uvMinU
		);

		//リトライ選択肢画像
		int retryHandle = loader.GetGraphic(ResourceLoader::GraphicID::ReTry);
		//タイトルに戻る選択肢画像
		int backTitleHandle = loader.GetGraphic(ResourceLoader::GraphicID::BackTitle);
		//カーソルが乗っているときのリトライ選択肢画像
		int retryOnCursorHandle = loader.GetGraphic(ResourceLoader::GraphicID::ReTryOnCursor);
		//カーソルが乗っているときのタイトルに戻る選択肢画像
		int backTitleOnCursorHandle = loader.GetGraphic(ResourceLoader::GraphicID::BackTitleOnCursor);

		//選択肢の描画
		//開く演出中(openProgressが1未満)は、まだワイプを考慮せず
		//通常画像2つを開く演出の範囲(uvMaxU/uvMinU)で描画する
		if (openProgress != 1.0f)
		{
			//リトライ選択肢を描画
			DrawGraphToShaderByCenter(
				wsize.m_width * retry_ratio.m_x,
				wsize.m_height * retry_ratio.m_y,
				select_graph_scale, retryHandle,
				1.0f,
				uvMaxU,
				uvMinU
			);
			//タイトルに戻る選択肢を描画
			DrawGraphToShaderByCenter(
				wsize.m_width * back_title_ratio.m_x,
				wsize.m_height * back_title_ratio.m_y,
				select_graph_scale, backTitleHandle,
				1.0f,
				uvMaxU,
				uvMinU
			);
		}
		//開く演出が終わっていたら、選ばれている方だけカーソルオン画像を
		//ワイプ進行度で重ね描きし、選ばれていない方は常にフル表示する
		else
		{
			switch (m_selectIndex)
			{
			case ClearSelect::ReTry:
			{
				//リトライ選択肢描画
				//もしゲームスタートのワイプ進行度が0より大きければ
				//カーソルが乗っている画像を左から進行度の範囲だけ切り取って描画
				if (m_wipeProgress[static_cast<int>(ClearSelect::ReTry)] > 0.0f)
				{
					//リトライ選択肢を描画
					DrawGraphToShaderByCenter(
						wsize.m_width * retry_ratio.m_x,
						wsize.m_height * retry_ratio.m_y,
						select_graph_scale, retryOnCursorHandle,
						1.0f,
						m_wipeProgress[static_cast<int>(ClearSelect::ReTry)]
					);
				}
				//タイトルに戻る選択肢を描画
				DrawGraphToShaderByCenter(
					wsize.m_width * back_title_ratio.m_x,
					wsize.m_height * back_title_ratio.m_y,
					select_graph_scale, backTitleHandle,
					1.0f
				);
				break;
			}
			case ClearSelect::BackTitle:
			{
				//タイトルに戻るのほうのワイプ進行度が0より大きければ
				//タイトルに戻るの方に進行度を適用する
				if (m_wipeProgress[static_cast<int>(ClearSelect::BackTitle)] > 0.0f)
				{
					//タイトルに戻る選択肢を描画
					DrawGraphToShaderByCenter(
						wsize.m_width * back_title_ratio.m_x,
						wsize.m_height * back_title_ratio.m_y,
						select_graph_scale, backTitleOnCursorHandle,
						1.0f, m_wipeProgress[static_cast<int>(ClearSelect::BackTitle)]
					);
				}
				//リトライ選択肢を描画
				DrawGraphToShaderByCenter(
					wsize.m_width * retry_ratio.m_x,
					wsize.m_height * retry_ratio.m_y,
					select_graph_scale, retryHandle,
					1.0f
				);
				break;
			}
			}
		}
	}

	SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, ShaderRegister::glitch_buffer);
	SetUsePixelShader(-1);

	//Aボタン画像
	int aButtonHandle = loader.GetGraphic(ResourceLoader::GraphicID::ButtonA);
	//決定のテキスト画像
	int decideHandle = loader.GetGraphic(ResourceLoader::GraphicID::DecideText);
	//次へ のテキスト画像
	int nextHandle = loader.GetGraphic(ResourceLoader::GraphicID::NextText);

	//二つの画像を描画
	//Aボタン画像
	DrawRotaGraph(
		wsize.m_width * a_button_pos.m_x,
		wsize.m_height * a_button_pos.m_y,
		a_button_scale, 0.0, aButtonHandle, true
	);

	//次へボタンを押してからは決定画像に切り替え
	if (m_isPushNextButton)
	{
		//決定のテキスト画像
		DrawRotaGraph(
			wsize.m_width * decide_graph_pos.m_x,
			wsize.m_height * decide_graph_pos.m_y,
			decide_graph_scale, 0.0, decideHandle, true
		);
	}
	else
	{
		//次へ のテキスト画像
		DrawRotaGraph(
			wsize.m_width * next_graph_pos.m_x,
			wsize.m_height *next_graph_pos.m_y,
			next_graph_scale, 0.0, nextHandle, true
		);
	}
}

void ClearScene::DrawResultText(int fontHandle)
{
	SetDrawScreen(m_textRenderTargetH);
	//前フレームの文字をクリア
	ClearDrawScreen();

	//ウィンドウサイズ
	Size wsize = Application::GetInstance().GetWindowSize();

	//敵を倒した数
	std::wstring killCountWString = std::to_wstring(static_cast<int>(m_currentKillCount));
	int killCountWidth = GetDrawStringWidthToHandle(killCountWString.c_str(), killCountWString.size(), fontHandle);
	DrawStringToHandle(
		wsize.m_width * kill_count_pos.m_x - killCountWidth,
		wsize.m_height * kill_count_pos.m_y,
		killCountWString.c_str(), score_color, fontHandle
	);
	//クリアタイム
	std::wstring clearTimeWString = std::to_wstring(static_cast<int>(m_currentClearTime));
	int clearTimeWidth = GetDrawStringWidthToHandle(clearTimeWString.c_str(), clearTimeWString.size(), fontHandle);
	DrawStringToHandle(
		wsize.m_width * clear_time_pos.m_x - clearTimeWidth,
		wsize.m_height * clear_time_pos.m_y,
		clearTimeWString.c_str(), score_color, fontHandle
	);
	//被弾回数
	std::wstring hitCountWString = std::to_wstring(static_cast<int>(m_currentHitCount));
	int hitCountWidth = GetDrawStringWidthToHandle(hitCountWString.c_str(), hitCountWString.size(), fontHandle);
	DrawStringToHandle(
		wsize.m_width * hit_count_pos.m_x - hitCountWidth,
		wsize.m_height * hit_count_pos.m_y,
		hitCountWString.c_str(), hit_count_color, fontHandle
	);

	//スコア
	std::wstring scoreWString = std::to_wstring(static_cast<int>(m_currentScore));
	int scoreWidth = GetDrawStringWidthToHandle(scoreWString.c_str(), scoreWString.size(), fontHandle);
	DrawStringToHandle(
		wsize.m_width * score_pos.m_x - scoreWidth,
		wsize.m_height * score_pos.m_y,
		scoreWString.c_str(), score_color, fontHandle
	);

	//通常描画に戻す
	SetDrawScreen(DX_SCREEN_BACK);

	//------ぼかし---------
	SetDrawScreen(m_textGlowH);
	ClearDrawScreen();
	//敵を倒した数
	DrawStringToHandle(
		wsize.m_width * kill_count_pos.m_x - killCountWidth,
		wsize.m_height * kill_count_pos.m_y,
		killCountWString.c_str(), score_color, fontHandle
	);
	//クリアタイム
	DrawStringToHandle(
		wsize.m_width * clear_time_pos.m_x - clearTimeWidth,
		wsize.m_height * clear_time_pos.m_y,
		clearTimeWString.c_str(), score_color, fontHandle
	);
	//被弾回数
	DrawStringToHandle(
		wsize.m_width * hit_count_pos.m_x - hitCountWidth,
		wsize.m_height * hit_count_pos.m_y,
		hitCountWString.c_str(), hit_count_color, fontHandle
	);
	//スコア
	DrawStringToHandle(
		wsize.m_width * score_pos.m_x - scoreWidth,
		wsize.m_height * score_pos.m_y,
		scoreWString.c_str(), score_color, fontHandle
	);

	//通常描画に戻す
	SetDrawScreen(DX_SCREEN_BACK);
	//ぼかしをかける
	GraphFilter(
		m_textGlowH,
		DX_GRAPH_FILTER_GAUSS,
		blur_range, blur_strength
	);
	//ぼかしをかける
	GraphFilter(
		m_textGlowH,
		DX_GRAPH_FILTER_GAUSS,
		blur_range, blur_strength
	);
}
