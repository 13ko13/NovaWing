#include "BossEnemy.h"
#include "Constants/ShaderRegister.h"

namespace
{
	//モデルのサイズ
	const Vector3 model_scale = { 3.0f,3.0f,3.0f };
	//敵自身の球の当たり判定の半径
	constexpr float col_radius = 132.0f;
	//フレームの番号がいくつ進むごとに足のフレームがあるか
	constexpr int leg_frame_stride = 6;
	//右脚の最初のフレーム番号
	constexpr int first_left_leg_idx = 30;
	//片側の足の本数
	constexpr int one_side_leg_num = 3;
}

BossEnemy::BossEnemy(
	const std::weak_ptr<Player> pPlayer,
	const ResourceLoader::ModelID Id,
	const std::shared_ptr<BulletManager> pBulletManager,
	std::weak_ptr<CameraBase> pCamera, const Vector3& pos): 
	Charactor(Id,pCamera),
    m_pPlayer(pPlayer),
	m_pBulletManager(pBulletManager),
	m_colSphere(pos)
{
	//位置を反映
	SetPos(pos);
}

BossEnemy::~BossEnemy()
{
}

void BossEnemy::OnInit()
{
	//シェーダーに渡す定数バッファを作成
	CreateShaderBuffers();

	//最初の右足の位置(3本)を取得
	for (int i = 1; i < static_cast<int>(LegIndex::BackLeft); i++)
	{
		//脚の位置を取得
		VECTOR rightLegPos = MV1GetFramePosition(
			m_modelHandle, leg_frame_stride * i
		);
		//配列に格納
		m_legPositions.push_back(rightLegPos);
	}
	//最初の左足の位置(3本)を取得
	for (int i = 0; i < static_cast<int>(LegIndex::FrontRight); i++)
	{
		//脚の位置を取得
		VECTOR leftLegPos = MV1GetFramePosition(
			m_modelHandle, first_left_leg_idx + leg_frame_stride * i
		);
		//配列に格納
		m_legPositions.push_back(leftLegPos);
	}
}

void BossEnemy::Update()
{
	//キャラクタークラス共通の処理
	Charactor::Update();

	//当たり判定の更新
	m_colSphere.Update(m_pos, col_radius);

	//脚の位置を取得するが、インデックスの途中で不要なものが入っているので
	//二回に分けて取得する

	//最初の右足の位置(3本)を取得
	for (int i = 1; i < static_cast<int>(LegIndex::BackLeft); i++)
	{
		//脚の位置を取得
		VECTOR rightLegPos = MV1GetFramePosition(
			m_modelHandle, leg_frame_stride * i
		);
		//配列に格納
		m_legPositions[i - 1] = rightLegPos;
	}
	//最初の左足の位置(3本)を取得
	for (int i = 0; i < static_cast<int>(LegIndex::FrontRight); i++)
	{
		//脚の位置を取得
		VECTOR leftLegPos = MV1GetFramePosition(
			m_modelHandle, first_left_leg_idx + leg_frame_stride * i
		);
		//配列に格納
		m_legPositions[i + one_side_leg_num] = leftLegPos;
	}
}

void BossEnemy::Draw()
{
	//モデルに行列を適用
	ApplyMatrix(model_scale, m_pos, m_rotation, m_modelHandle);

	//シェーダに渡すカメラ情報を更新してから
	UpdateShaderMatrixData();

	//モデル描画
	MV1DrawModel(m_modelHandle);
}

void BossEnemy::TakeDamage(int damage)
{
}

void BossEnemy::DrawEnemy()
{
	//ResourceLoaderから浮遊敵の法線マップを取得
	//ResourceLoaderのインスタンスを取得
	//const ResourceLoader& resourceLoader = ResourceLoader::GetInstance();
	////法線マップ取得
	//const int normGraphH = resourceLoader.GetGraphic(
	//	ResourceLoader::GraphicID::EnemyNormalMap);
	////エミッションマップを取得
	//const int emissionGraphH = resourceLoader.GetGraphic(
	//	ResourceLoader::GraphicID::EnemyEmissionMap);
	////ディゾルブ用ノイズ
	//int noiseHandle = ResourceLoader::GetInstance().GetGraphic(
	//	ResourceLoader::GraphicID::DissolveNoise
	//);

	////ハンドルとレジスタ番号をセットで保持しておく
	//std::vector<std::pair<int, int>> textures;
	//textures.push_back({ ShaderRegister::tex_normal,normGraphH });//法線マップ
	//textures.push_back({ ShaderRegister::tex_metalic,-1 });//メタリックマップはないので-1
	//textures.push_back({ ShaderRegister::tex_emission,emissionGraphH });//エミッション
	//textures.push_back({ ShaderRegister::tex_noise,noiseHandle });//ノイズテクスチャ

	////DrawWithLightingにペアの配列を渡してモデルを描画
	//DrawWithLighting(textures);
}
