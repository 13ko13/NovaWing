#include <string>

#include "ClearScene.h"
#include "Manager/InputManager.h"
#include "Scene/TitleScene.h"
#include "SceneController.h"
#include "Main/Application.h"
#include "Utility/GraphShaderDraw.h"
#include "Constants/ShaderRegister.h"

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
	constexpr float templete_size = 0.75f;

	//敵を倒した数を表示する座標
	const Vector2 kill_count_pos = Vector2(900.0f, 200.0f);
	//クリアタイムを表示する座標
	const Vector2 clear_time_pos = Vector2(900.0f, 300.0f);
	//被弾回数を表示する座標
	const Vector2 hit_count_pos = Vector2(900.0f, 410.0f);
	//スコアを表示する座標
	const Vector2 score_pos = Vector2(900.0f, 537.0f);

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
	const Vector2 a_button_pos = Vector2(1150.0f, 690.0f);
	//つぎへのテキスト画像の描画座標
	const Vector2 next_graph_pos = Vector2(1190.0f, 690.0f);
	//決定のテキスト画像の描画座標
	const Vector2 decide_graph_pos = Vector2(1190.0f, 690.0f);
	//Aボタン画像のサイズ
	constexpr double a_button_scale = 0.3;
	//つぎへのテキスト画像のサイズ
	constexpr double next_graph_scale = 0.3;
	//決定のテキスト画像のサイズ
	constexpr double decide_graph_scale = 0.3;
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

	//テンプレートの開く演出用のフレーム更新
	if (!m_controller.GetFade().IsFading() &&
		!m_isPushNextButton)
	{
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
		}
	}

	////下入力で選択肢を下に移動(indexを増やす) 
	//if (input.IsTriggered(InputEvent::down))
	//{
	//	//選択肢の最大数で割った余りを取ることで、
	//	//選択肢の範囲内に収める
	//	m_selectIndex =
	//		static_cast<ClearSelect>(
	//			(static_cast<int>(m_selectIndex) + 1) %
	//			static_cast<int>(ClearSelect::SelectMax));
	//}
	////上入力で選択肢を上に移動(indexを減らす)
	//if (input.IsTriggered(InputEvent::up))
	//{
	//	//選択肢の最大数で割った余りを取ることで、
	//	//選択肢の範囲内に収める
	//	m_selectIndex =
	//		static_cast<ClearSelect>(
	//			(static_cast<int>(m_selectIndex) - 1 + static_cast<int>(ClearSelect::SelectMax)) %
	//			static_cast<int>(ClearSelect::SelectMax));
	//}
	////決定入力で選択肢を決定する
	//if (input.IsTriggered(InputEvent::ok))
	//{
	//	switch (m_selectIndex)
	//	{
	//	case ClearSelect::BackTitle:
	//	{
	//		//タイトルシーンに遷移する
	//		m_controller.ChangeScene(
	//			std::make_shared<TitleScene>(
	//				m_controller), scene_change_frame);
	//		break;
	//	}
	//	case ClearSelect::ExitGame:
	//	{
	//		//ゲームを終了する
	//		Application::GetInstance().RequestExit();
	//		break;
	//	}
	//	}
	//}
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
			1.0, backgroundH,
			1.0f,
			uvMaxU,
			uvMinU
		);

		//リトライ選択肢画像
		int retryHandle = loader.GetGraphic(ResourceLoader::GraphicID::ReTry);

		//リトライ選択肢を描画
		DrawGraphToShaderByCenter(
			wsize.m_width * 0.5f, wsize.m_height * 0.5f,
			0.5, retryHandle,
			1.0f,
			uvMaxU,
			uvMinU
		);
	}

	SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, ShaderRegister::glitch_buffer);
	SetUsePixelShader(-1);

	//Aボタン画像
	int aButtonHandle = loader.GetGraphic(ResourceLoader::GraphicID::ButtonA);
	//決定のテキスト画像
	int decideHandle = loader.GetGraphic(ResourceLoader::GraphicID::DecideText);
	//次へ のテキスト画像
	int nextHandle = loader.GetGraphic(ResourceLoader::GraphicID::NextText);
	//カーソルが乗っているときのリトライ選択肢画像
	int retryOnCursorHandle = loader.GetGraphic(ResourceLoader::GraphicID::ReTryOnCursor);

	//二つの画像を描画
	//Aボタン画像
	DrawRotaGraph(
		a_button_pos.m_x,
		a_button_pos.m_y,
		a_button_scale, 0.0, aButtonHandle, true
	);

	//次へボタンを押してからは決定画像に切り替え
	if (m_isPushNextButton)
	{
		//決定のテキスト画像
		DrawRotaGraph(
			decide_graph_pos.m_x,
			decide_graph_pos.m_y,
			decide_graph_scale, 0.0, decideHandle, true
		);
	}
	else
	{
		//次へ のテキスト画像
		DrawRotaGraph(
			next_graph_pos.m_x,
			next_graph_pos.m_y,
			next_graph_scale, 0.0, nextHandle, true
		);
	}


	//とりあえず左上に選択肢を表示する
	//選択中の選択肢に矢印を表示する
	switch (m_selectIndex)
	{
	case ClearSelect::BackTitle:
	{
		DrawFormatString(0, 15, 0xffffff, cursor_text);
		DrawFormatString(15, 15, 0xff0000, back_title_text);
		DrawFormatString(15, 30, 0xffffff, exit_game_text);
		break;
	}
	case ClearSelect::ExitGame:
	{
		DrawFormatString(0, 30, 0xffffff, cursor_text);
		DrawFormatString(15, 15, 0xffffff, back_title_text);
		DrawFormatString(15, 30, 0xff0000, exit_game_text);
		break;
	}
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
		kill_count_pos.m_x - killCountWidth, kill_count_pos.m_y,
		killCountWString.c_str(), score_color, fontHandle
	);
	//クリアタイム
	std::wstring clearTimeWString = std::to_wstring(static_cast<int>(m_currentClearTime));
	int clearTimeWidth = GetDrawStringWidthToHandle(clearTimeWString.c_str(), clearTimeWString.size(), fontHandle);
	DrawStringToHandle(
		clear_time_pos.m_x - clearTimeWidth, clear_time_pos.m_y,
		clearTimeWString.c_str(), score_color, fontHandle
	);
	//被弾回数
	std::wstring hitCountWString = std::to_wstring(static_cast<int>(m_currentHitCount));
	int hitCountWidth = GetDrawStringWidthToHandle(hitCountWString.c_str(), hitCountWString.size(), fontHandle);
	DrawStringToHandle(
		hit_count_pos.m_x - hitCountWidth, hit_count_pos.m_y,
		hitCountWString.c_str(), hit_count_color, fontHandle
	);

	//スコア
	std::wstring scoreWString = std::to_wstring(static_cast<int>(m_currentScore));
	int scoreWidth = GetDrawStringWidthToHandle(scoreWString.c_str(), scoreWString.size(), fontHandle);
	DrawStringToHandle(
		score_pos.m_x - scoreWidth, score_pos.m_y,
		scoreWString.c_str(), score_color, fontHandle
	);

	//通常描画に戻す
	SetDrawScreen(DX_SCREEN_BACK);

	//------ぼかし---------
	SetDrawScreen(m_textGlowH);
	ClearDrawScreen();
	//敵を倒した数
	DrawStringToHandle(
		kill_count_pos.m_x - killCountWidth, kill_count_pos.m_y,
		killCountWString.c_str(), score_color, fontHandle
	);
	//クリアタイム
	DrawStringToHandle(
		clear_time_pos.m_x - clearTimeWidth, clear_time_pos.m_y,
		clearTimeWString.c_str(), score_color, fontHandle
	);
	//被弾回数
	DrawStringToHandle(
		hit_count_pos.m_x - hitCountWidth, hit_count_pos.m_y,
		hitCountWString.c_str(), hit_count_color, fontHandle
	);
	//スコア
	DrawStringToHandle(
		score_pos.m_x - scoreWidth, score_pos.m_y,
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
