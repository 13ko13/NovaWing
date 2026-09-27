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
#include "Game/GameObjects/Bullet/ReflectedBullet.h"
#include "Game/Collision/BulletCollider.h"

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

	//反射弾をもとの弾のどれぐらいの速度で反射させるか
	constexpr float reflect_bullet_speed_rate = 5.5f;
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
	for (const std::weak_ptr<ReflectedBullet>& weakBullet : pBulletManager->GetReflectedBullets())
	{
		std::shared_ptr<ReflectedBullet> pBullet = weakBullet.lock();
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
	//カウンターが敵弾を跳ね返した場合は専用処理を行う
	bool isCounterVsEnemyBullet;
	if((a.GetTag() == ColliderTag::Counter && b.GetTag() == ColliderTag::EnemyBullet) ||
	(a.GetTag() == ColliderTag::EnemyBullet && b.GetTag() == ColliderTag::Counter))
	{
		isCounterVsEnemyBullet = true;
	}
	else isCounterVsEnemyBullet = false;

	//カウンター成功の場合
	if(isCounterVsEnemyBullet)
	{
		//タグが敵弾の方のコライダーを保持しておく
		ICollider* pBulletCollider = &a;
		if(a.GetTag() != ColliderTag::EnemyBullet)
		{
			pBulletCollider = &b;
		}
		//敵弾反射の関数にそのコライダーを渡す
		ReflectEnemyBullet(static_cast<BulletCollider&>(*pBulletCollider));

		//お互いの反応はそれぞれのコライダーに任せる
		a.OnCollision(b);
		b.OnCollision(a);
		return;
	}

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

void CollisionManager::ReflectEnemyBullet(BulletCollider& bulletCollider)
{
	//実体は敵弾なのでキャストする
	EnemyBullet* pEnemyBullet = dynamic_cast<EnemyBullet*>(&bulletCollider.GetOwner());
	//仮に敵弾じゃない場合は処理を行わない
	if (pEnemyBullet == nullptr) return;

	//発射元の敵を取得できなければ反射しない
	std::shared_ptr<EnemyBase> pShooter = pEnemyBullet->GetShooter().lock();
	if (pShooter == nullptr) return;

	//現在位置から発射元の敵への方向を反射方向とする
	Vector3 pos = pEnemyBullet->GetPos();
	Vector3 toShooterDir = (pShooter->GetPos() - pos).Normalized();
	//元の弾より速い速度で反射
	float speed = pEnemyBullet->GetVel().Length();

	//反射弾用の情報を入れる
	ReflectedBullet::ReflectBulletData data;
	data.pos = pos;
	data.vel = toShooterDir * speed * reflect_bullet_speed_rate;
	data.attackPower = pEnemyBullet->GetAttackPower();
	data.pTarget = pShooter;
	data.homingStrength = 1.0f;
	data.pCamera = m_pCamera;

	//弾の管理者に反射用の弾を生み出させる
	m_pBulletManager.lock()->CreateReflectedBullet(data);
}
