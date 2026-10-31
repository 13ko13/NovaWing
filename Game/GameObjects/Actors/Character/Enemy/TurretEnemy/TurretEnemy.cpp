#include "TurretEnemy.h"
#include "Game/Collision/EnemyCollider.h"
#include "Manager/DebugManager.h"
#include "Constants/ShaderRegister.h"

namespace
{
	const Vector3 model_scale = Vector3(2.0f, 2.0f, 2.0f);
}

TurretEnemy::TurretEnemy(const TurretEnemyData& data) :
	EnemyBase(data.modelID, data.pCamera, data.pPlayer, data.pBulletManager,
	data.pSoundManager, data.pEffectManager)
{
	//当たり判定の生成
	m_colSphere = std::make_shared<SphereShape>(data.pos,m_colRadius);
	m_pCollider = std::make_unique<EnemyCollider>(*this, ColliderTag::Floating);

	//位置を設定
	SetPos(data.pos);
}

TurretEnemy::~TurretEnemy()
{
}

void TurretEnemy::OnInit()
{
	//シェーダーを使用する準備
	CreateShaderBuffers();
}

void TurretEnemy::Update()
{
	//死亡待機中なら管理用のフレームを更新
	if (m_isDying)
	{
		m_dyingFrame++;
		//死亡待機が一定フレームを超えたら死亡
		if (m_dyingFrame > m_trueDeadFrame)
		{
			//完全死亡
			OnEnemyDead();
		}
	}
}

void TurretEnemy::Draw()
{
	//モデルに行列を適用
	ApplyMatrix(model_scale, m_pos, m_rotation, m_modelHandle);

	//シェーダに渡すカメラ情報を更新してから
	UpdateShaderMatrixData();

	//敵の描画
	DrawEnemy();


#ifdef _DEBUG
	if (DebugManager::GetInstance().IsDebugDrawEnabled())
	{
		//当たり判定の描画
		m_colSphere->Draw(0xffffff);
	}
#endif
}

std::vector<std::shared_ptr<SphereShape>> TurretEnemy::GetCollisionSpheres() const
{
	if (m_isDying) return {};
	else return { m_colSphere };
}

std::vector<ICollider*> TurretEnemy::GetColliders()
{
	//生ポインタを取得して返す
	return { m_pCollider.get() };
}

void TurretEnemy::DrawEnemy()
{
	//ResourceLoaderから浮遊敵の法線マップを取得
	//ResourceLoaderのインスタンスを取得
	const ResourceLoader& resourceLoader = ResourceLoader::GetInstance();
	//法線マップ取得
	const int normGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::FloatingNormalMap);
	//エミッションマップを取得
	const int emissionGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::FloatingEmissionMap);
	//ディゾルブ用ノイズ
    int noiseHandle = ResourceLoader::GetInstance().GetGraphic(
        ResourceLoader::GraphicID::DissolveNoise
    );

	//ハンドルとレジスタ番号をセットで保持しておく
    std::vector<std::pair<int,int>> textures;
    textures.push_back({ShaderRegister::tex_normal,normGraphH});//法線マップ
    textures.push_back({ShaderRegister::tex_metalic,-1});//メタリックマップはないので-1
    textures.push_back({ShaderRegister::tex_emission,emissionGraphH});//エミッション
    textures.push_back({ShaderRegister::tex_noise,noiseHandle});//ノイズテクスチャ

    //DrawWithLightingにペアの配列を渡してモデルを描画
    DrawWithLighting(textures);
}
