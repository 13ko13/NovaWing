#pragma once
#include "Game/GameObjects/Actors/Character/Enemy/EnemyBase.h"

class Player;
class BulletManager;
class CameraBase;
class SoundManager;
class EffectManager;
class SphereShape;
class EnemyCollider;
/// <summary>
/// 建物の上に固定された砲台の敵
/// </summary>
class TurretEnemy : public EnemyBase
{
public:
	struct TurretEnemyData
	{
		ResourceLoader::ModelID modelID;//モデルID
		std::weak_ptr<Player> pPlayer;//プレイヤー
		std::weak_ptr<BulletManager> pBulletManager;//弾の管理者
		std::weak_ptr<CameraBase> pCamera;//カメラ
		Vector3 pos;//位置
		std::weak_ptr<SoundManager> pSoundManager;//サウンドマネージャー
		std::weak_ptr<EffectManager> pEffectManager;//エフェクトマネージャー
	};

	TurretEnemy(const TurretEnemyData& data);
	virtual ~TurretEnemy();

	void OnInit() override;
	void Update() override;
	void Draw() override;

	//当たり判定を返す
	std::vector<std::shared_ptr<SphereShape>> GetCollisionSpheres() const override;
	//当たり判定インターフェースを返す
	std::vector<ICollider*> GetColliders() override;

private:
	//状態ごとの更新
	void UpdateWait(const Vector3& playerPos);
	void UpdateAim(const Vector3& playerPos);

	//プレイヤーを攻撃できる状態かを返す
	//プレイヤーと近くて、画面に映っているのであればtrue
	bool CanAttack(const Vector3& playerPos) const;

	//プレイヤーに向けて弾を撃つ
	void Shoot(const Vector3& playerPos);

	//敵を描画
	void DrawEnemy();

private:
	//ステート
	enum class State
	{
		Wait,//待機
		Aim,//呼び動作
	};
	State m_state = State::Wait;

	//今のステートになってからの時間
	float m_stateTimer = 0.0f;
	//当たり判定の球
	std::shared_ptr<SphereShape> m_colSphere;
	//当たり判定インターフェース
	std::unique_ptr<EnemyCollider> m_pCollider;
};
