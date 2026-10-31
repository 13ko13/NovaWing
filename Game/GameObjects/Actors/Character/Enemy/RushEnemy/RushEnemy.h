#pragma once
#include "Game/GameObjects/Actors/Character/Enemy/EnemyBase.h"
#include "Manager/ResourceLoader.h"

class Player;
class BulletManager;
class CameraBase;
class SoundManager;
class EffectManager;
class SphereShape;
class EnemyCollider;
class RushEnemy : public EnemyBase
{
public:
	struct RushEnemyData
	{
		ResourceLoader::ModelID modelID;//モデルID
		std::weak_ptr<Player> pPlayer;//プレイヤー
		std::weak_ptr<BulletManager> pBulletManager;//弾の管理者
		std::weak_ptr<CameraBase> pCamera;//カメラ
		Vector3 pos;//位置
		std::weak_ptr<SoundManager> pSoundManager;//サウンドマネージャー
		std::weak_ptr<EffectManager> pEffectManager;//エフェクトマネージャー
	};

	RushEnemy(const RushEnemyData& data);
	virtual ~RushEnemy();

	void OnInit() override;
	void Update() override;
	void Draw() override;

	//当たり判定を返す
	std::vector<std::shared_ptr<SphereShape>> GetCollisionSpheres() const override;
	//当たり判定インターフェースを返す
	std::vector<ICollider*> GetColliders() override;

	//プレイヤーに当たった時の処理
	void OnHitPlayer() override;

private:
	//状態ごとの更新
	void UpdateAppear(const Vector3& playerPos);
	void UpdateCharge(const Vector3& playerPos);
	void UpdateRush(const Vector3& playerPos);
	//敵を描画
	void DrawEnemy();
	//モデルの正面を毎フレーム向きたい方向に回す
	void LookAtDir(const Vector3& dir);

private:
	//ステート
	enum class State
	{
		Appear,//出現
		Charge,//予備動作
		Rush,//突進
	};
	State m_state = State::Appear;

	//今のステートになってからの時間
	float m_stateTimer = 0.0f;
	//プレイヤーから見た待機場所のずれ
	Vector3 m_standbyOffset;
	//予備動作中の震えの中心位置
	Vector3 m_chargeCenter;
	//突進する方向
	Vector3 m_rushDir;
	//前のフレームのプレイヤーのZ
	float m_prevPlayerZ = 0.0f;
	//当たり判定の球
	std::shared_ptr<SphereShape> m_colSphere;
	//当たり判定インターフェース
	std::unique_ptr<EnemyCollider> m_pCollider;
};
