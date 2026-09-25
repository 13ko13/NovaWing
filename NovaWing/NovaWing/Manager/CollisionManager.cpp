#include <map>
#include <algorithm>

#include "CollisionManager.h"
#include "Manager/BulletManager.h"
#include "Game/GameObjects/Bullet/PlayerBullet.h"
#include "Game/GameObjects/Bullet/ChargeBullet.h"
#include "Game/GameObjects/Bullet/EnemyBullet.h"
#include "Charactor/Player/Player.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/EnemyBase.h"
#include "Game/GameObjects/Actors/Rock/Rock.h"
#include "Game/GameObjects/Camera/GameCamera.h"
#include "Game/GameObjects/Actors/Charactor/Enemy/BossEnemy/BossEnemy.h"
#include "Game/Collision/ICollider.h"
#include "Game/Collision/SphereShape.h"
#include "Game/Collision/EnemyCollider.h"
#include "Game/Collision/RockCollider.h"
#include "Game/Collision/BossBeamCollider.h"

namespace
{
	//プレイヤーがダメージを食らった時のカメラを揺らす力
	constexpr float shake_power = 7.0f;
	//プレイヤーがダメージを食らった時にどのくらいの時間カメラを揺らすか
	constexpr int shake_frame = 18;

	//判定を行うタグの組み合わせ
	struct TagPair
	{
		ColliderTag a;
		ColliderTag b;
	};
	//上から順に処理する
	//弾は当たると消えるので、ボスのダメージ判定を無敵判定より先に置いてダメージ判定を優先させる
	const TagPair hit_pairs[] =
	{
		{ ColliderTag::PlayerBullet, ColliderTag::Enemy },
		{ ColliderTag::PlayerBullet, ColliderTag::Worm },
		{ ColliderTag::EnemyBullet,ColliderTag::Counter },
		{ ColliderTag::EnemyBullet, ColliderTag::Player },
		{ ColliderTag::PlayerBullet, ColliderTag::BossDamage },
		{ ColliderTag::PlayerBullet, ColliderTag::BossShield },
		{ ColliderTag::BossBeam, ColliderTag::Counter },
		{ ColliderTag::BossBeam, ColliderTag::Player },
		{ ColliderTag::Worm, ColliderTag::Player },
		{ ColliderTag::Rock, ColliderTag::Player },
	};

	//2つのコライダーの形状のどれかが重なっているか
	bool IsHit(const ICollider& a, const ICollider& b)
	{
		std::vector<std::shared_ptr<ColliderShape>> shapesA = a.GetCollision();
		std::vector<std::shared_ptr<ColliderShape>> shapesB = b.GetCollision();
		for (const std::shared_ptr<ColliderShape>& shapeA : shapesA)
		{
			for (const std::shared_ptr<ColliderShape>& shapeB : shapesB)
			{
				//今は球同士の判定しかない
				if (shapeA->GetShape() != Shape::Sphere || shapeB->GetShape() != Shape::Sphere) continue;

				const SphereShape& sphereA = static_cast<const SphereShape&>(*shapeA);
				const SphereShape& sphereB = static_cast<const SphereShape&>(*shapeB);
				if (sphereA.HitCollision(sphereB)) return true;
			}
		}
		return false;
	}

	//触れ続けている間は1回しかダメージを与えない相手なら、そのダメージ源の情報を作る
	bool TryGetDamageSource(const ICollider& collider, DamageSource& outSource)
	{
		//タグで相手の具体的な型が決まるのでstatic_castしてよい
		switch (collider.GetTag())
		{
		case ColliderTag::Rock:
			outSource = { DamageSourceType::Rock, static_cast<const RockCollider&>(collider).GetOwnerID() };
			return true;
		case ColliderTag::Worm:
			outSource = { DamageSourceType::Worm, static_cast<const EnemyCollider&>(collider).GetOwnerID() };
			return true;
		case ColliderTag::BossBeam:
			outSource = { DamageSourceType::Beam, static_cast<const BossBeamCollider&>(collider).GetOwnerID() };
			return true;
		default:
			return false;
		}
	}
}

CollisionManager::CollisionManager(
	const std::weak_ptr<Player> pPlayer,
	const std::weak_ptr<BulletManager> pBulletManager,
	const std::weak_ptr<GameCamera> pCamera,
	const std::weak_ptr<BossEnemy> pBoss) :
	m_pPlayer(pPlayer),
	m_pBulletManager(pBulletManager),
	m_pCamera(pCamera),
	m_pBoss(pBoss)
{

}

CollisionManager::~CollisionManager()
{

}

void CollisionManager::Register(std::shared_ptr<EnemyBase> pEnemy)
{
	m_pEnemies.push_back(pEnemy);
}

void CollisionManager::RegisterRock(std::shared_ptr<Rock> pRock)
{
	//渡された岩を配列として管理する
	m_pRocks.push_back(pRock);
}

void CollisionManager::Update()
{
	//shared_ptrに変換
	std::shared_ptr<BulletManager> pBulletManager = m_pBulletManager.lock();
	std::shared_ptr<Player> pPlayer = m_pPlayer.lock();
	std::shared_ptr<BossEnemy> pBoss = m_pBoss.lock();

	//判定中にオブジェクトが解放されないよう、このフレームの間shared_ptrで保持しておく
	std::vector<std::shared_ptr<GameObject>> keepAlive;
	//タグごとにコライダーを分けて持つ
	std::map<ColliderTag, std::vector<ICollider*>> colliders;
	auto addCollider = [&colliders](ICollider& collider)
		{
			colliders[collider.GetTag()].push_back(&collider);
		};

	//プレイヤー
	addCollider(pPlayer->GetHitCollider());
	//プレイヤーのカウンター判定
	addCollider(pPlayer->GetCounterCollider());

	//弾
	for (const std::weak_ptr<PlayerBullet>& weakBullet : pBulletManager->GetPlayerBullets())
	{
		std::shared_ptr<PlayerBullet> pBullet = weakBullet.lock();
		if (!pBullet) continue;
		keepAlive.push_back(pBullet);
		addCollider(pBullet->GetCollider());
	}
	for (const std::weak_ptr<ChargeBullet>& weakBullet : pBulletManager->GetChargeBullets())
	{
		std::shared_ptr<ChargeBullet> pBullet = weakBullet.lock();
		if (!pBullet) continue;
		keepAlive.push_back(pBullet);
		addCollider(pBullet->GetCollider());
	}
	for (const std::weak_ptr<EnemyBullet>& weakBullet : pBulletManager->GetEnemyBullets())
	{
		std::shared_ptr<EnemyBullet> pBullet = weakBullet.lock();
		if (!pBullet) continue;
		keepAlive.push_back(pBullet);
		addCollider(pBullet->GetCollider());
	}

	//敵(1体が複数のコライダーを持つことがある)
	for (std::weak_ptr<EnemyBase>& weakEnemy : m_pEnemies)
	{
		std::shared_ptr<EnemyBase> pEnemy = weakEnemy.lock();
		if (!pEnemy) continue;
		keepAlive.push_back(pEnemy);
		for (ICollider* pCollider : pEnemy->GetColliders())
		{
			addCollider(*pCollider);
		}
	}
	for (ICollider* pCollider : pBoss->GetColliders())
	{
		addCollider(*pCollider);
	}

	//岩
	for (std::weak_ptr<Rock>& weakRock : m_pRocks)
	{
		std::shared_ptr<Rock> pRock = weakRock.lock();
		if (!pRock) continue;
		keepAlive.push_back(pRock);
		addCollider(pRock->GetCollider());
	}

	//決められた組み合わせだけ判定する
	for (const TagPair& pair : hit_pairs)
	{
		for (ICollider* pA : colliders[pair.a])
		{
			for (ICollider* pB : colliders[pair.b])
			{
				//当たって消えた弾などは、同じフレームでも以降は判定しない
				if (!pA->IsCollisionActive() || !pB->IsCollisionActive()) continue;
				if (!IsHit(*pA, *pB)) continue;

				OnHit(*pA, *pB);
			}
		}
	}

	//前フレームは当たっていて今フレームは当たっていないダメージ源は、離れたことを記録する
	for (const DamageSource& source : m_hitSourcesPrevFrame)
	{
		if (m_hitSourcesThisFrame.find(source) == m_hitSourcesThisFrame.end())
		{
			pPlayer->OnLeaveDamaging(source);
		}
	}
	m_hitSourcesPrevFrame = m_hitSourcesThisFrame;
	m_hitSourcesThisFrame.clear();

	//死んでいる敵は配列から消す
	m_pEnemies.erase(
	std::remove_if(m_pEnemies.begin(), m_pEnemies.end(),
		[](const std::weak_ptr<EnemyBase>& pEnemy)
		{
			return pEnemy.lock() == nullptr;
		}),
	m_pEnemies.end()
	);
}

void CollisionManager::OnHit(ICollider& a, ICollider& b)
{
	//判定の組み合わせ上、プレイヤーが関わる衝突はすべてプレイヤーの被弾
	bool isPlayerHit = a.GetTag() == ColliderTag::Player || b.GetTag() == ColliderTag::Player;

	if (isPlayerHit)
	{
		//多段ヒット防止
		ICollider& other = (a.GetTag() == ColliderTag::Player) ? b : a;
		DamageSource source;
		if (TryGetDamageSource(other, source))
		{
			//当たったことを記録
			m_hitSourcesThisFrame.insert(source);

			//既にこのダメージ源に当たっている場合は処理を行わない
			std::shared_ptr<Player> pPlayer = m_pPlayer.lock();
			if (pPlayer->IsTakingDamageFrom(source)) return;
			pPlayer->StartTakingDamage(source);
		}
	}

	//お互いの反応はそれぞれのコライダーに任せる
	a.OnCollision(b);
	b.OnCollision(a);

	if (isPlayerHit)
	{
		//カメラを揺らす
		m_pCamera.lock()->OnShake(shake_power, shake_frame);
	}
}
