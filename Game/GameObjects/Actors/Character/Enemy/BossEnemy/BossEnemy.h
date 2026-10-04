#pragma once
#include "Game/GameObjects/Actors/Character/Enemy/EnemyBase.h"
#include "Manager/ResourceLoader.h"
#include "Game/Collision/SphereShape.h"
#include "Game/Collision/BossDamageCollider.h"
#include "Game/Collision/BossShieldCollider.h"
#include "Game/Collision/BossBeamCollider.h"
#include "Utility/ModelAnimator.h"
#include "Game/Collision/BossBeamTipCollider.h"

class Player;
class BulletManager;
class IBossEnemyState;
class EnemyFactory;
class SoundManager;
class EffectManager;
class BossEnemy : public EnemyBase
{
public:
	//ボス生成に必要な情報
	struct BossEnemyData
	{
		std::weak_ptr<Player> pPlayer;
		ResourceLoader::ModelID Id = ResourceLoader::ModelID::None;
		std::weak_ptr<BulletManager> pBulletManager;
		std::weak_ptr<CameraBase> pCamera;
		std::weak_ptr<SoundManager> pSoundManager;
		std::weak_ptr<EffectManager> pEffectManager;
		Vector3 pos;
		int health = 0;
	};

	BossEnemy(BossEnemyData& data);

	~BossEnemy();

	void OnInit() override; // 初期化処理
	void Update() override;//更新処理
	void Draw() override;//描画処理
	void TakeDamage(int damage) override;//被弾処理

	//敵生産工場をセット
	void SetEnemyFactory(std::weak_ptr<EnemyFactory> pEnemyFactory) { m_pEnemyFactory = pEnemyFactory; }

	//敵生産工場取得
	std::weak_ptr<EnemyFactory> GetEnemyFactory() const {
		return m_pEnemyFactory;
	}

	//ボスのモデルハンドル取得
	int GetModelHandle() const { return m_modelHandle; }

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
	std::vector<ICollider*> GetColliders() override { return { &m_damageCollider, &m_shieldCollider, &m_beamCollider, &m_beamRightTipCollider, &m_beamLeftTipCollider }; }

	//現在のステートを返す
	std::shared_ptr<IBossEnemyState> GetCurrentState() const { return m_pState; }

	//ボス出現が終了しているかをセットする
	void SetIsBossAppear(bool isAppear) { m_isAppear = isAppear; }
	//ボス出現が終了しているかを返す
	bool IsBossAppear() const { return m_isAppear; }

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
	std::shared_ptr<IBossEnemyState> m_pState;
	//敵生産工場
	std::weak_ptr<EnemyFactory> m_pEnemyFactory;
	//音のマネージャー
	std::weak_ptr<SoundManager> m_pSoundManager;
	//エフェクトのマネージャー
	std::weak_ptr<EffectManager> m_pEffectManager;

	//プレイヤーの弾が当たった時に、無敵判定する部分
	std::shared_ptr<SphereShape> m_invincibleHitCol = std::make_shared<SphereShape>();
	//プレイヤーの弾が当たった時のダメージ判定する部分
	std::shared_ptr<SphereShape> m_damageCol = std::make_shared<SphereShape>();

	//ボスが出現完了しているか
	//ゲームシーン側からセットさせる
	bool m_isAppear = false;
	//一番最初の着地時のみに使用するフラグ
	bool m_isFirstLanding = false;

	//死亡待機状態をどのぐらい続けるかを計測
	int m_dyingFrame = 0;

	//足音のクールタイム計測
	int m_footstepCT = 0;

	//当たり判定インターフェース
	BossDamageCollider m_damageCollider;//ダメージ判定
	BossShieldCollider m_shieldCollider;//無敵判定
	BossBeamCollider m_beamCollider;//ビーム
	BossBeamTipCollider m_beamRightTipCollider;//右のビームの先端判定
	BossBeamTipCollider m_beamLeftTipCollider;//左のビームの先端判定
};