#pragma once
#include <memory>
#include <vector>

#include "Game/GameObjects/Actors/Character/Enemy/Boss/BossBase.h"
#include "Game/Collision/GunshipBossNetCollider.h"

class ModelAnimator;
class IGunshipBossState;
class SphereShape;
class GunshipBossWeakCollider;
class CapsuleShape;
class GunshipBossBodyCollider;
class BulletBase;
class GunshipBoss : public BossBase
{
public:
	struct GunshipBossData : public BossBaseData
	{
		Vector3 pos;
	};

	//攻撃の種類
	enum class AttackType
	{
		Net,//網
		Rocket,//ロケット

		Max,//攻撃の種類の数
	};

	GunshipBoss(GunshipBossData& data);
	virtual ~GunshipBoss();
	
	//初期化処理
	void OnInit() override;
	//更新
	void Update() override;
	//描画
	void Draw() override;

	//アニメーターを返す
	ModelAnimator& GetAnimator() { return *m_pAnimator; }
	//現在のステートを返す
	std::shared_ptr<IGunshipBossState> GetCurrentState() const { return m_pState; }

	//全ての当たり判定を配列で返す
	std::vector<ICollider*> GetColliders() override;

	//弱点の球を返す
	std::shared_ptr<SphereShape> GetWeakSphereLeft() const { return m_pWeakSphereLeft; }//左
	std::shared_ptr<SphereShape> GetWeakSphereRight() const { return m_pWeakSphereRight; }//右

	//現在弱点が出ているかを返す
	bool IsWeakOpen() const;

	//胴体のカプセルを返す
	std::shared_ptr<CapsuleShape> GetBodyCapsule() const { return m_pBodyCapsule; }

	//弾のダメージを受ける
	void TakeBulletDamage(const BulletBase& bullet, bool isPodHit);

	//ロックオン可能か
	bool CanLockOn() const override;
	//ロックオンできる位置
	Vector3 GetLockablePos() const override;

	//前の攻撃の種類を取得
	AttackType GetPrevAttackType() const { return m_prevAttackType; }
	//現在同じ攻撃が何回続いたかを取得
	int GetSameAttackCount() const { return m_sameAttackCount; }
	//今回の攻撃を記録する
	void RecordAttack(AttackType currentAttackType);

	//バレットマネージャーを取得
	std::weak_ptr<BulletManager> GetBulletManager() const { return m_pBulletManager; }
	//カメラを返す
	std::weak_ptr<CameraBase> GetCamera() const { return m_pCamera; }

private:
	//ステートの更新
	void UpdateState();
	//弱点の球の初期化
	void InitWeakSphere();
	//胴体のカプセルの初期化
	void InitBodyCapsule();
	//胴体のカプセルを胴体のボーンの位置に合わせる
	void UpdateBodyCapsule();

private:
	//第二形態になるときに表示する壊れたポッドのボーン番号
	int m_brokenPodBoneIdx = -1;
	//アニメーター
	std::unique_ptr<ModelAnimator> m_pAnimator;
	//現在のステート
	std::shared_ptr<IGunshipBossState> m_pState;

	//網攻撃のコライダー
	GunshipBossNetCollider m_netCol;
	//弱点(放熱中のポッド)のコライダー
	std::unique_ptr<GunshipBossWeakCollider> m_pWeakCol;

	//弱点となるボーンの番号
	int m_weakBoneIdxLeft = -1;
	int m_weakBoneIdxRight = -1;

	//弱点となる球のコライダー
	//左
	std::shared_ptr<SphereShape> m_pWeakSphereLeft;
	//右
	std::shared_ptr<SphereShape> m_pWeakSphereRight;

	//胴体のコライダー
	std::unique_ptr<GunshipBossBodyCollider> m_pBodyCol;
	//胴体のボーンの番号
	int m_bodyBoneIdx = -1;
	//胴体の当たり判定のカプセル(横向き)
	std::shared_ptr<CapsuleShape> m_pBodyCapsule;

	//まだボスに与えていないダメージ
	float m_storedDamage = 0.0f;

	//前の攻撃
	AttackType m_prevAttackType = AttackType::Max;
	//同じ攻撃が続いた回数
	int m_sameAttackCount = 0;
};