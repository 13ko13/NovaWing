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
class GunshipBoss : public BossBase
{
public:
	struct GunshipBossData : public BossBaseData
	{
		Vector3 pos;
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

	//ロックオン可能か
	bool CanLockOn() const override;
	//ロックオンできる位置
	Vector3 GetLockablePos() const override;

private:
	//ステートの更新
	void UpdateState();
	//弱点の球の初期化
	void InitWeakSphere();
	//胴体のカプセルの初期化
	void InitBodyCapsule();
	//胴体のカプセルを胴体のフレームの位置に合わせる
	void UpdateBodyCapsule();

private:
	//第二形態になるときに表示する壊れたポッドのフレーム番号
	int m_brokenPodFrameIdx = -1;
	//アニメーター
	std::unique_ptr<ModelAnimator> m_pAnimator;
	//現在のステート
	std::shared_ptr<IGunshipBossState> m_pState;

	//網攻撃のコライダー
	GunshipBossNetCollider m_netCol;
	//弱点(放熱中のポッド)のコライダー
	std::unique_ptr<GunshipBossWeakCollider> m_pWeakCol;

	//弱点となるフレームの番号
	int m_weakFrameIdxLeft = -1;
	int m_weakFrameIdxRight = -1;

	//弱点となる球のコライダー
	//左
	std::shared_ptr<SphereShape> m_pWeakSphereLeft;
	//右
	std::shared_ptr<SphereShape> m_pWeakSphereRight;

	//胴体のコライダー
	std::unique_ptr<GunshipBossBodyCollider> m_pBodyCol;
	//胴体のフレームの番号
	int m_bodyFrameIdx = -1;
	//胴体の当たり判定のカプセル(横向き)
	std::shared_ptr<CapsuleShape> m_pBodyCapsule;
};