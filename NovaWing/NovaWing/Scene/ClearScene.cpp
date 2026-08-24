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

	//テンプレート画像のサイズ
	constexpr float templete_size = 0.75f;

	//敵を倒した数を表示する座標
	const Vector2 kill_count_pos = Vector2(800.0f, 200.0f);
	//クリアタイムを表示する座標
	const Vector2 clear_time_pos = Vector2(800.0f, 300.0f);
	//被弾回数を表示する座標
	const Vector2 hit_count_pos = Vector2(800.0f, 410.0f);

	//文字をぼかす範囲
	constexpr int blur_range = 16;
	//ぼかしの強さ
	constexpr int blur_strength = 1400;

	//ぼかしの透明度
	constexpr int blur_alpha = 255;
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

	//リザルト情報を描画
	int fontHandle = ResourceLoader::GetInstance().GetFont(
		ResourceLoader::FontID::Result);

	//ウィンドウサイズ
	Size wsize = Application::GetInstance().GetWindowSize();

	//文字にもシェーダをかけるために1枚の画像として
	//文字のみ描画された画像を作る
	m_textRenderTargetH = MakeScreen(wsize.m_width, wsize.m_height,true);
	SetDrawScreen(m_textRenderTargetH);

	//敵を倒した数
	DrawFormatStringToHandle(
		kill_count_pos.m_x, kill_count_pos.m_y,
		0x00ffaa, fontHandle,
		L"%d", m_resultData.defeatedEnemyCount);
	//クリアタイム
	DrawFormatStringToHandle(
		clear_time_pos.m_x, clear_time_pos.m_y,
		0x00ffaa, fontHandle,
		L"%d", m_resultData.clearTime / 60);
	//被弾回数
	DrawFormatStringToHandle(
		hit_count_pos.m_x, hit_count_pos.m_y,
		0xe13c3c, fontHandle,
		L"%d", -m_resultData.hitCount);
	//通常描画に戻す
	SetDrawScreen(DX_SCREEN_BACK);

	//------ぼかし---------

	//発光用のぼかし画像を作る
	m_textGlowH = MakeScreen(wsize.m_width, wsize.m_height, true);
	SetDrawScreen(m_textGlowH);
	//敵を倒した数
	DrawFormatStringToHandle(
		kill_count_pos.m_x, kill_count_pos.m_y,
		0x00ffaa, fontHandle,
		L"%d", m_resultData.defeatedEnemyCount);
	//クリアタイム
	DrawFormatStringToHandle(
		clear_time_pos.m_x, clear_time_pos.m_y,
		0x00ffaa, fontHandle,
		L"%d", m_resultData.clearTime / 60);
	//被弾回数
	DrawFormatStringToHandle(
		hit_count_pos.m_x, hit_count_pos.m_y,
		0xe13c3c, fontHandle,
		L"%d", -m_resultData.hitCount);

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

void ClearScene::Update()
{
	//InputManagerのインスタンスを取得
	InputManager& input = InputManager::GetInstance();
	//フレーム更新
	m_frame++;
	m_pCBuffGlitchData->time = m_frame * time_speed;//シェーダ用のフレームに変換して渡す
	UpdateShaderConstantBuffer(m_cbufferGlitch);

	//テンプレートの開く演出用のフレーム更新
	if (!m_controller.GetFade().IsFading())
	{
		m_templeteOpenFrame++;
		if (m_templeteOpenFrame > templete_opne_max_frame)
		{
			m_templeteOpenFrame = templete_opne_max_frame;
		}
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
	}
	//決定入力で選択肢を決定する
	if (input.IsTriggered(InputEvent::ok))
	{
		switch (m_selectIndex)
		{
		case ClearSelect::BackTitle:
		{
			//タイトルシーンに遷移する
			m_controller.ChangeScene(
				std::make_shared<TitleScene>(
					m_controller), scene_change_frame);
			break;
		}
		case ClearSelect::ExitGame:
		{
			//ゲームを終了する
			Application::GetInstance().RequestExit();
			break;
		}
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
	DrawFormatString(x, y, 0xffff00, game_clear_text);

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

	//ぼかし画像を先に加算合成で描画する
	SetDrawBlendMode(DX_BLENDMODE_ADD, blur_alpha);
	//オフスクリーン画像を描画する
	DrawGraphToShaderByCenter(
		wsize.m_width * 0.5f,
		wsize.m_height * 0.5f,
		1.0,
		m_textGlowH
	); 
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 255);

	//オフスクリーン画像を描画する
	DrawGraphToShaderByCenter(
		wsize.m_width * 0.5f,
		wsize.m_height * 0.5f,
		1.0,
		m_textRenderTargetH
	);

	SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, ShaderRegister::glitch_buffer);
	SetUsePixelShader(-1);

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
