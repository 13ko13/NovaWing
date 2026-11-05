#pragma once
#include "Game/GameObjects/Actors/Character/Enemy/EnemyBase.h"

class Player;
class BulletManager;
class CameraBase;
class SoundManager;
class EffectManager;
class SphereShape;
class EnemyCollider;
class ButterflyEnemy : public EnemyBase
{
public:
	struct ButterflyEnemyData
	{
		ResourceLoader::ModelID modelID;//モデルID
		std::weak_ptr<Player> pPlayer;//プレイヤー
		std::weak_ptr<BulletManager> pBulletManager;//弾の管理者
		std::weak_ptr<CameraBase> pCamera;//カメラ
		Vector3 pos;//位置
		std::weak_ptr<SoundManager> pSoundManager;//サウンドマネージャー
		std::weak_ptr<EffectManager> pEffectManager;//エフェクトマネージャー
	};
	
	ButterflyEnemy(const ButterflyEnemyData& data);
	virtual ~ButterflyEnemy();

	void OnInit() override;
	void Update() override;
	void Draw() override;
	//当たり判定を返す
	std::vector<std::shared_ptr<SphereShape>> GetCollisionSpheres() const override;
	//当たり判定インターフェースを返す
	std::vector<ICollider*> GetColliders() override;

private:
	//敵を描画
	void DrawEnemy();

private:
	//当たり判定の球
	std::shared_ptr<SphereShape> m_colSphere;
	//当たり判定インターフェース
	std::unique_ptr<EnemyCollider> m_pCollider;
	//プレイヤーから見た待機場所のずれ
	Vector3 m_standbyOffset;
	//前のフレームのプレイヤーのZ
	float m_prevPlayerZ = 0.0f;
};
