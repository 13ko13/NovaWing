#include "AutoPlayWorld.h"
#ifdef _DEBUG
#include "Manager/GameObjectManager.h"
#include "Game/GameObjects/GameObject.h"
#include "Game/GameObjects/Actors/Character/Player/Player.h"
#include "Game/GameObjects/Actors/Character/Enemy/EnemyBase.h"
#include "Game/GameObjects/Actors/Character/Enemy/BossEnemy/BossEnemy.h"
#include "Game/GameObjects/Bullet/EnemyBullet.h"
#include "Game/GameObjects/Actors/Rock/Rock.h"
#include "Game/Collision/ICollider.h"
#include "Game/Collision/SphereShape.h"

namespace
{
	//これより大きい移動量は別物を同じものと見間違えたとみなして0にする
	constexpr float max_estimated_speed = 200.0f;

	//GameObjectManagerのprivateな配列を、ゲーム側を書き換えずに読むための仕組み
	//(テンプレートの明示的インスタンス化ではアクセス指定が無視される、というC++の規則を使う)
	using GameObjectList = std::vector<std::shared_ptr<GameObject>>;
	using GameObjectListPtr = GameObjectList GameObjectManager::*;

	template <typename Tag, GameObjectListPtr Member>
	struct PrivateListAccessor
	{
		friend GameObjectListPtr GetListMember(Tag) { return Member; }
	};

	//登録済みのオブジェクトの配列
	struct GameObjectsTag
	{
		friend GameObjectListPtr GetListMember(GameObjectsTag);
	};
	template struct PrivateListAccessor<GameObjectsTag, &GameObjectManager::m_pGameObjects>;

	//登録待ちのオブジェクトの配列(Initで登録されたばかりのものはまだこちらにいる)
	struct HoldGameObjectsTag
	{
		friend GameObjectListPtr GetListMember(HoldGameObjectsTag);
	};
	template struct PrivateListAccessor<HoldGameObjectsTag, &GameObjectManager::m_pHoldGameObject>;

	//当たり判定の形から球だけを取り出す
	std::vector<std::shared_ptr<SphereShape>> GetSpheres(const ICollider& collider)
	{
		std::vector<std::shared_ptr<SphereShape>> spheres;
		for (const std::shared_ptr<ColliderShape>& shape : collider.GetCollision())
		{
			if (!shape || shape->GetShape() != Shape::Sphere) continue;
			spheres.push_back(std::static_pointer_cast<SphereShape>(shape));
		}
		return spheres;
	}
}

AutoPlayWorld::AutoPlayWorld()
{
}

AutoPlayWorld::~AutoPlayWorld()
{
}

void AutoPlayWorld::Refresh()
{
	ReleaseObjects();
	m_currentPositions.clear();

	GameObjectManager& manager = GameObjectManager::GetInstance();
	const GameObjectList& objects = manager.*GetListMember(GameObjectsTag{});
	const GameObjectList& holdObjects = manager.*GetListMember(HoldGameObjectsTag{});

	//危険物を球ごとに追加する
	auto addColliderThreats = [this](ICollider& collider, ThreatKind kind)
		{
			if (!collider.IsCollisionActive()) return;
			for (const std::shared_ptr<SphereShape>& sphere : GetSpheres(collider))
			{
				//球はそれぞれ作り直されずに動くので、球のアドレスで見分ける
				Threat threat;
				threat.kind = kind;
				threat.pos = sphere->GetPos();
				threat.vel = EstimateVelocity(sphere.get(), threat.pos);
				threat.radius = sphere->GetRadius();
				threat.key = sphere.get();
				m_threats.push_back(threat);
			}
		};

	auto readObject = [&](const std::shared_ptr<GameObject>& pObject)
		{
			if (!pObject) return;

			//プレイヤー
			if (std::shared_ptr<Player> pPlayer = std::dynamic_pointer_cast<Player>(pObject))
			{
				m_pPlayer = pPlayer;
				return;
			}

			//敵弾(当たった直後で死んでいるものも、被弾の原因を調べるために残す)
			if (std::shared_ptr<EnemyBullet> pBullet = std::dynamic_pointer_cast<EnemyBullet>(pObject))
			{
				Threat threat;
				threat.kind = ThreatKind::EnemyBullet;
				threat.pos = pBullet->GetSphere()->GetPos();
				threat.vel = pBullet->GetVel();
				threat.radius = pBullet->GetSphere()->GetRadius();
				threat.key = pBullet.get();
				threat.isDead = pBullet->IsDead();
				m_threats.push_back(threat);
				return;
			}

			//岩
			if (std::shared_ptr<Rock> pRock = std::dynamic_pointer_cast<Rock>(pObject))
			{
				m_hasRocks = true;
				addColliderThreats(pRock->GetCollider(), ThreatKind::Rock);
				return;
			}

			//敵(ボスを含む)
			std::shared_ptr<EnemyBase> pEnemy = std::dynamic_pointer_cast<EnemyBase>(pObject);
			if (!pEnemy || pEnemy->IsDead()) return;

			std::shared_ptr<BossEnemy> pBoss = std::dynamic_pointer_cast<BossEnemy>(pEnemy);
			if (pBoss)
			{
				m_pBoss = pBoss;
			}
			else
			{
				//ステージ構成の記録用に、出現前の敵も位置だけ覚えておく
				Enemy marker;
				marker.pos = pEnemy->GetPos();
				for (ICollider* pCollider : pEnemy->GetColliders())
				{
					if (pCollider && pCollider->GetTag() == ColliderTag::Worm) marker.isWorm = true;
				}
				m_allEnemyPositions.push_back(marker);
			}

			for (ICollider* pCollider : pEnemy->GetColliders())
			{
				if (!pCollider) continue;
				switch (pCollider->GetTag())
				{
				case ColliderTag::Worm:
					addColliderThreats(*pCollider, ThreatKind::Worm);
					break;
				case ColliderTag::BossBeam:
					addColliderThreats(*pCollider, ThreatKind::BossBeam);
					break;
				case ColliderTag::BossBeamTip:
					addColliderThreats(*pCollider, ThreatKind::BossBeamTip);
					break;
				default:
					break;
				}

				//ボス以外で、弾が当たる状態の敵は倒す対象にする
				bool isTargetTag = pCollider->GetTag() == ColliderTag::Floating ||
					pCollider->GetTag() == ColliderTag::Worm;
				if (isTargetTag && pCollider->IsCollisionActive() && !pEnemy->IsDying())
				{
					std::vector<std::shared_ptr<SphereShape>> spheres = GetSpheres(*pCollider);
					if (spheres.empty()) continue;
					Enemy enemy;
					enemy.pos = spheres.front()->GetPos();//ワームは頭を狙う
					enemy.radius = spheres.front()->GetRadius();
					enemy.isWorm = pCollider->GetTag() == ColliderTag::Worm;
					m_enemies.push_back(enemy);
				}
			}
		};

	for (const std::shared_ptr<GameObject>& pObject : objects)
	{
		readObject(pObject);
	}
	for (const std::shared_ptr<GameObject>& pObject : holdObjects)
	{
		readObject(pObject);
	}

	m_prevPositions = m_currentPositions;
}

void AutoPlayWorld::ReleaseObjects()
{
	m_pPlayer.reset();
	m_pBoss.reset();
	m_threats.clear();
	m_enemies.clear();
	m_allEnemyPositions.clear();
	m_hasRocks = false;
}

Vector3 AutoPlayWorld::EstimateVelocity(const void* key, const Vector3& pos)
{
	m_currentPositions[key] = pos;

	auto it = m_prevPositions.find(key);
	if (it == m_prevPositions.end()) return Vector3();

	Vector3 vel = pos - it->second;
	if (vel.Length() > max_estimated_speed) return Vector3();
	return vel;
}
#endif
