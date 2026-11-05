#include "ButterflyEnemy.h"
#include "Game/Collision/EnemyCollider.h"
#include "Game/Collision/SphereShape.h"
#include "Game/GameObjects/Actors/Character/Player/Player.h"
#include "Main/Application.h"
#include "Manager/DebugManager.h"
#include "Constants/ShaderRegister.h"

namespace
{
	//画面外のどれだけ外側から入ってくるか
	// constexpr float appear_distance =6000.0f;
	constexpr float appear_distance = 0.0f;
	//モデルの大きさ
	const Vector3 model_scale = Vector3(2.0f, 2.0f, 2.0f);
}

ButterflyEnemy::ButterflyEnemy(const ButterflyEnemyData& data) :
	EnemyBase(data.modelID, data.pCamera, data.pPlayer,
		data.pBulletManager, data.pSoundManager, data.pEffectManager)
{

	//当たり判定の生成
	m_colSphere = std::make_shared<SphereShape>(data.pos, 0.0f);
	m_pCollider = std::make_unique<EnemyCollider>(*this, ColliderTag::ButterflyEnemy);


	//待機位置のずれ
	//プレイヤーの位置からどれだけ離れた位置に待機するか
	Vector3 playerPos = m_pPlayer.lock()->GetPos();//プレイヤーの位置
	m_standbyOffset = data.pos - playerPos;

	//初期位置設定
	//最初は待機位置から画面の外側に離れたところに置く
	Vector3 outward = Vector3(m_standbyOffset.x, m_standbyOffset.y, 0.0f);
	if (outward.Length() == 0.0f) outward = Vector3(0.0f, 1.0f, 0.0f);
	else outward.Normalize();
	SetPos(data.pos + outward * appear_distance);
}

ButterflyEnemy::~ButterflyEnemy()
{
}

void ButterflyEnemy::OnInit()
{
	//シェーダーで描くための準備を行う
	CreateShaderBuffers();
	//今のプレイヤーのZを前のフレームのZ座標にいれる
	Vector3 playerPos = m_pPlayer.lock()->GetPos();
	m_prevPlayerZ = playerPos.z;
}

void ButterflyEnemy::Update()
{
}

void ButterflyEnemy::Draw()
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

std::vector<std::shared_ptr<SphereShape>> ButterflyEnemy::GetCollisionSpheres() const
{
	return std::vector<std::shared_ptr<SphereShape>>();
}

std::vector<ICollider*> ButterflyEnemy::GetColliders()
{
	return std::vector<ICollider*>();
}

void ButterflyEnemy::DrawEnemy()
{
	//ResourceLoaderから突進敵の法線マップを取得
	//ResourceLoaderのインスタンスを取得
	const ResourceLoader& resourceLoader = ResourceLoader::GetInstance();
	//法線マップ取得
	const int normGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::RushEnemyNormalMap);
	//エミッションマップを取得
	const int emissionGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::RushEnemyEmissionMap);
	//メタリック
	const int metalicGraphH = resourceLoader.GetGraphic(
		ResourceLoader::GraphicID::RushEnemyMetalicMap);

	//ディゾルブ用ノイズ
	int noiseHandle = ResourceLoader::GetInstance().GetGraphic(
		ResourceLoader::GraphicID::DissolveNoise
	);

	//ハンドルとレジスタ番号をセットで保持しておく
	std::vector<std::pair<int, int>> textures;
	textures.push_back({ ShaderRegister::tex_normal,normGraphH });//法線マップ
	textures.push_back({ ShaderRegister::tex_metalic,metalicGraphH });//メタリックマップ
	textures.push_back({ ShaderRegister::tex_emission,emissionGraphH });//エミッション
	textures.push_back({ ShaderRegister::tex_noise,noiseHandle });//ノイズテクスチャ

	//DrawWithLightingにペアの配列を渡してモデルを描画
	DrawWithLighting(textures);
}
