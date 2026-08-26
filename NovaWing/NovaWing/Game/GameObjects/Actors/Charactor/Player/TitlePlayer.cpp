#include <DxLib.h>

#include "TitlePlayer.h"
#include "Constants/ShaderRegister.h"
#include "Manager/LightingManager.h"

namespace
{
	//前進フェーズで進む速度
	constexpr float move_speed = 10.0f;
	//ブーストフェーズで進む速度
	constexpr float boost_speed = 30.0f;

	// モデルのサイズ
	const Vector3 model_scale = { 0.3f, 0.3f, 0.3f };

	//宙返りを終了とするフレーム
	constexpr int somersault_max_frame = 110;

	//宙返り中の移動速度
	constexpr float somersault_move_speed = 20.0f;

	//プレイヤーの初期位置
	const Vector3 first_pos = Vector3(0.0f, 300.0f, 3100.0f);
}

TitlePlayer::TitlePlayer(
	ResourceLoader::ModelID modelID,
	std::weak_ptr<CameraBase> pCamera):
	Actor(modelID,pCamera)
{
}

TitlePlayer::~TitlePlayer()
{
}

void TitlePlayer::OnInit()
{
	//モデルが逆向きなのでY軸180度回転させておく
	m_rotation = Quaternion(Vector3(0.0f, 1.0f, 0.0f), DX_PI_F);

	//プレイヤーの初期位置はz以外0で、zは0より少し後ろ
	m_pos = first_pos;

	//シェーダに渡す定数バッファを作成
	CreateShaderBuffers();
}

void TitlePlayer::Update()
{
	switch (m_phase)
	{
	case Phase::Forward:
		//前に進む
		SetVel(Vector3(0.0f,0.0f,move_speed));
		break;
	case Phase::Somersault:
	{
		//宙返りの経過フレームを進める
		m_somersaultFrame++;

		//進行度(0～1)を計算
		float progress = static_cast<float>(m_somersaultFrame) / somersault_max_frame;
		//X軸回転角(0～360度)
		float targetAngleX = progress * DX_TWO_PI_F;

		//X軸周りに回転させる
		//モデルの回転補正を消してしまわないようにX軸とY軸の回転を合成させる
		Quaternion initRotation = Quaternion(Vector3(0.0f, 1.0f, 0.0f), DX_PI_F);
		Quaternion somersaultRotation = Quaternion(Vector3(1.0f, 0.0f, 0.0f), -targetAngleX);
		m_rotation = somersaultRotation * initRotation;

		//回転角に応じた速度を作る(縦に一回転する軌道)
		Vector3 vel;
		vel.m_y = sinf(targetAngleX) * somersault_move_speed;
		vel.m_z = cosf(-targetAngleX) * somersault_move_speed;
		SetVel(vel);

		break;
	}
	case Phase::Boost:
		//前に進む
		SetVel(Vector3(0.0f, 0.0f, boost_speed));
		break;
	}

	//位置に速度を足す
	m_pos += m_velocity;
}

void TitlePlayer::Draw()
{
	//モデルに行列を適用
	ApplyMatrix(model_scale, m_pos, m_rotation, m_modelHandle);

	//シェーダに渡すカメラ情報を更新
	UpdateShaderMatrixData();

	//ResourceLoaderからプレイヤーの法線マップ等を取得
	const ResourceLoader& resourceLoader = ResourceLoader::GetInstance();
	const int normGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::PlayerNormalMap);
	const int metalicGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::PlayerMetalicMap);
	const int emissionGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::PlayerEmissionMap);

	//ハンドルとレジスタ番号をセットで保持
	std::vector<std::pair<int, int>> textures;
	textures.push_back({ ShaderRegister::tex_normal, normGraphH });
	textures.push_back({ ShaderRegister::tex_metalic, metalicGraphH });
	textures.push_back({ ShaderRegister::tex_emission, emissionGraphH });
	textures.push_back({ ShaderRegister::tex_noise, -1 });

	//DrawWithLightingに渡してモデルを描画
	DrawWithLighting(textures);

#ifdef _DEBUG
	//プレイヤーの位置表示
	DrawFormatString(0, 15, 0xffffff, L"playerPosZ : %f", m_pos.m_z);
#endif
}

void TitlePlayer::StartSomersault()
{
	//宙返りステートに遷移
	m_phase = Phase::Somersault;
	m_somersaultFrame = 0;
}

void TitlePlayer::StartBoost()
{
	//ブーストステートに遷移
	m_phase = Phase::Boost;
}

bool TitlePlayer::IsSomersaultEnd()
{
	return m_somersaultFrame >= somersault_max_frame;
}
