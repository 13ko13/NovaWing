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
}

ClearScene::ClearScene(SceneController& controller, const ClearResultData& data) :
    Scene(controller),
	m_resultData(data)
{
}

ClearScene::~ClearScene()
{
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
	m_templeteOpenFrame++;
	if (m_templeteOpenFrame > templete_opne_max_frame)
	{
		m_templeteOpenFrame = templete_opne_max_frame;
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
	SetUsePixelShader(-1);
	SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, ShaderRegister::glitch_buffer);

	//一旦リザルト情報を描画
	/*DrawFormatString(x, 300, 0xffffff, L"敵を倒した数 : %d", m_resultData.defeatedEnemyCount);
	DrawFormatString(x, 315, 0xffffff, L"クリアタイム : %d", m_resultData.clearTime / 60);
	DrawFormatString(x, 330, 0xffffff, L"被弾回数 : %d", m_resultData.hitCount);*/

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
