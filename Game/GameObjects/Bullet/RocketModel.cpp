#include <DxLib.h>
#include <cmath>

#include "RocketModel.h"
#include "Constants/ShaderRegister.h"

namespace
{
	//モデルの大きさ
	const Vector3 model_scale = Vector3(2.0f, 2.0f, 2.0f);

	//モデルの正面の向き
	const Vector3 model_forward = Vector3(0.0f, 0.0f, -1.0f);

	//モデルの正面と向きたい方向がほぼ同じかの閾値
	constexpr float forward_and_dir_threshold = 0.9999f;
}

RocketModel::RocketModel(std::weak_ptr<CameraBase> pCamera) :
	Actor(ResourceLoader::ModelID::Rocket, pCamera)
{
	//シェーダーに渡す定数バッファを作成
	CreateShaderBuffers();
}

RocketModel::~RocketModel()
{
	//作った定数バッファを消す
	DeleteShaderConstantBuffer(m_cbufferMatrix);
	DeleteShaderConstantBuffer(m_cbufferCamera);
}

void RocketModel::Update()
{
}

void RocketModel::Draw()
{
	//モデルに行列を適用
	ApplyMatrix(model_scale, m_pos, m_rotation, m_modelHandle);

	//シェーダに渡すカメラ情報を更新
	UpdateShaderMatrixData();

	//ResourceLoaderのインスタンスを取得
	const ResourceLoader& resourceLoader = ResourceLoader::GetInstance();
	//法線マップ
	const int normGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::RocketNormalMap);
	//メタリックマップ
	const int metalicGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::RocketMetalicMap);
	//エミッションマップ
	const int emissionGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::RocketEmissionMap);

	//ハンドルとレジスタ番号をセットで保持しておく
	std::vector<std::pair<int, int>> textures;
	textures.push_back({ ShaderRegister::tex_normal,normGraphH });//法線マップ
	textures.push_back({ ShaderRegister::tex_metalic,metalicGraphH });//メタリックマップ
	textures.push_back({ ShaderRegister::tex_emission,emissionGraphH });//エミッション
	textures.push_back({ ShaderRegister::tex_noise,-1 });//ノイズテクスチャはなし

	//モデルを描画
	DrawWithLighting(textures);
}

void RocketModel::LookAtDir(const Vector3& dir)
{
	//向きたい方向
	Vector3 to = dir.Normalized();
	//正面と向きたい方向がどれだけ同じ向きか
	float dot = Vector3::Dot(model_forward, to);

	//正面と向きたい方向がほぼ同じ向きなら回さない
	if (dot > forward_and_dir_threshold)
	{
		m_rotation = Quaternion();
		return;
	}
	//真逆なら正面と直角なX軸で180度回転する
	if (dot < -forward_and_dir_threshold)
	{
		m_rotation = Quaternion(Vector3(1.0f, 0.0f, 0.0f), DX_PI_F);
		return;
	}

	//2本のベクトルに直角な軸を中心に、間の角度だけ回す
	Vector3 axis = Vector3::Cross(model_forward, to).Normalized();
	float angle = std::acosf(dot);
	m_rotation = Quaternion(axis, angle);
}
