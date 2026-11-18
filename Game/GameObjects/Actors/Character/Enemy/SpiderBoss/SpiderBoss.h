#pragma once
#include "Game/GameObjects/Actors/Character/Enemy/Boss/BossBase.h"
#include "Manager/ResourceLoader.h"
#include "Game/Collision/SphereShape.h"
#include "Game/Collision/SpiderBossDamageCollider.h"
#include "Game/Collision/SpiderBossShieldCollider.h"
#include "Game/Collision/SpiderBossBeamCollider.h"
#include "Utility/ModelAnimator.h"
#include "Game/Collision/SpiderBossBeamTipCollider.h"

class Player;
class BulletManager;
class ISpiderBossState;
class EnemyFactory;
class SoundManager;
class EffectManager;
class SpiderBoss : public BossBase
{
public:
	//ボス生成に必要な情報
	struct SpiderBossData
	{
		std::weak_ptr<Player> pPlayer;
		ResourceLoader::ModelID modelID = ResourceLoader::ModelID::None;
		std::weak_ptr<BulletManager> pBulletManager;
		std::weak_ptr<CameraBase> pCamera;
		std::weak_ptr<SoundManager> pSoundManager;
		std::weak_ptr<EffectManager> pEffectManager;
		Vector3 pos;
	};

	SpiderBoss(SpiderBossData& data);

	~SpiderBoss();

	void OnInit() override; // 初期化処理
	void Update() override;//更新処理
	void Draw() override;//描画処理
	void TakeDamage(int damage) override;//被弾処理
	//ボスのモデルハンドル取得
	int GetModelHandle() const { return m_modelHandle; }

	//敵生産工場をセット
	void SetEnemyFactory(std::weak_ptr<EnemyFactory> pEnemyFactory) { m_pEnemyFactory = pEnemyFactory; }
	//敵生産工場取得
	std::weak_ptr<EnemyFactory> GetEnemyFactory() const
	{
		return m_pEnemyFactory;
	}

	//プレイヤー取得
	std::weak_ptr<Player> GetPlayer() const { return m_pPlayer; }

	//自分の無敵当たり判定を返す
	std::shared_ptr<SphereShape> GetInvinsibleSphere() const { return m_invincibleHitCol; }
	//自分のダメージ当たり判定を返す
	std::shared_ptr<SphereShape> GetDamageSphere() const { return m_damageCol; }

	//ビームを出しているときにビームの当たり判定を返す
	std::vector<std::shared_ptr<SphereShape>> GetBeamSphereL() const;//左のビーム
	std::vector<std::shared_ptr<SphereShape>> GetBeamSphereR() const;//右のビーム

	//当たり判定インターフェースを返す(ダメージ・無敵・ビーム・ビーム先端)
	std::vector<ICollider*> GetColliders() override
	{
		return {
			&m_damageCollider,
			&m_shieldCollider,
			&m_beamCollider,
			&m_beamRightTipCollider,
			&m_beamLeftTipCollider
		};
	}

	//現在のステートを返す
	std::shared_ptr<ISpiderBossState> GetCurrentState() const { return m_pState; }

	//最初の着地が完了しているか
	bool IsFirstLanding() const { return m_isFirstLanding; }

	//ボスの無敵部分に当たった時の処理
	void OnHitInvincibleCol(const Position3& effectPos, const int attackPower);

	//サウンドマネージャー取得
	std::weak_ptr<SoundManager> GetSoundManager() const { return m_pSoundManager; }
	//エフェクトマネージャー取得
	std::weak_ptr<EffectManager> GetEffectManager() const { return m_pEffectManager; }

private:
	//敵の描画(シェーダ適応も含めた)
	void DrawEnemy();
	//すべての足の位置を取得
	void GetAllLegPos();
	//当たり判定球の初期化
	void InitCol(
		std::shared_ptr<SphereShape> sphere,
		const Vector3& pos,
		const float radius,
		const Vector3& offset
	);

private:
	//モデルの前フレームの足の位置(6箇所)
	std::vector<VECTOR> m_prevLegPositions;
	//モデルの今フレームの足の位置(6箇所)
	std::vector<VECTOR> m_currentLegPositions;

	//脚のボーン番号をこちらで決める
	enum class LegIndex
	{
		BackRight = 1,//右後ろ脚
		MiddleRight,//右の真ん中の脚
		FrontRight,//右前脚
		BackLeft,//左後ろ脚
		MiddleLeft,//左の真ん中の脚
		FrontLeft,//左前脚

		Max,//何個あるか
	};

	//モデル用のアニメーター
	ModelAnimator m_animator;

	//ステート
	std::shared_ptr<ISpiderBossState> m_pState;
	//敵生産工場
	std::weak_ptr<EnemyFactory> m_pEnemyFactory;

	//プレイヤーの弾が当たった時に、無敵判定する部分
	std::shared_ptr<SphereShape> m_invincibleHitCol = std::make_shared<SphereShape>();
	//プレイヤーの弾が当たった時のダメージ判定する部分
	std::shared_ptr<SphereShape> m_damageCol = std::make_shared<SphereShape>();

	//一番最初の着地時のみに使用するフラグ
	bool m_isFirstLanding = false;


	//足音のクールタイム計測
	int m_footstepCT = 0;

	//当たり判定インターフェース
	SpiderBossDamageCollider m_damageCollider;//ダメージ判定
	SpiderBossShieldCollider m_shieldCollider;//無敵判定
	SpiderBossBeamCollider m_beamCollider;//ビーム
	SpiderBossBeamTipCollider m_beamRightTipCollider;//右のビームの先端判定
	SpiderBossBeamTipCollider m_beamLeftTipCollider;//左のビームの先端判定

	//ダメージを食らった時の顔面点滅用のフレーム
	int m_damageBlinkingFrame = 0;
};