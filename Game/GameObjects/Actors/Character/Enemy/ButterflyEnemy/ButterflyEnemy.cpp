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
	constexpr float appear_distance =6000.0f;
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
}

std::vector<std::shared_ptr<SphereShape>> ButterflyEnemy::GetCollisionSpheres() const
{
	return std::vector<std::shared_ptr<SphereShape>>();
}

std::vector<ICollider*> ButterflyEnemy::GetColliders()
{
	return std::vector<ICollider*>();
}
