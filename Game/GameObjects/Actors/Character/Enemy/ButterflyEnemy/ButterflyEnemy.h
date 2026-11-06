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

	//出現中の更新処理
	void UpdateAppear(const Vector3& playerPos);
	//停止中の更新処理
	void UpdateStop(const Vector3& playerPos);
	//ダッシュ中の更新処理
	void UpdateDash(const Vector3& playerPos);
	//モデルを指定の方向に向かせる
	void LookAtDir(const Vector3& dir);

private:
	//ステート
	enum class State
	{
		Appear,//出現
		Stop,//停止
		Dash,//ダッシュ
		Charge,//予備動作
		Shoot,//発射
		Leave,//離脱
	};
	State m_state = State::Appear;

	//今のステートになってからの時間
	float m_stateTimer = 0.0f;
	//当たり判定の球
	std::shared_ptr<SphereShape> m_colSphere;
	//当たり判定インターフェース
	std::unique_ptr<EnemyCollider> m_pCollider;
	//プレイヤーから見た待機場所のずれ
	Vector3 m_standbyOffset;
	//前のフレームのプレイヤーのZ
	float m_prevPlayerZ = 0.0f;

	//ダッシュのスタート地点
	Vector3 m_dashStartPos;
	//ダッシュの終了地点
	Vector3 m_dashEndPos;

	//ダッシュした回数を数える
	int m_dashNum = 0;
};
