#pragma once
#include "Game/GameObjects/Actors/Charactor/Enemy/EnemyBase.h"
#include "Manager/ResourceLoader.h"
#include "Utility/Sphere.h"
#include "Utility/ModelAnimator.h"

class Player;
class BulletManager;
class IBossEnemyState;
class EnemyFactory;
class SoundManager;
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
	Sphere GetInvinsibleSphere() const { return m_invincibleHitCol; }
	//自分のダメージ当たり判定を返す
	Sphere GetDamageSphere() const { return m_damageCol; }

	//ビームを出しているときにビームの当たり判定を返す
	std::vector<Sphere> GetBeamSphereL() const;//左のビーム
	std::vector<Sphere> GetBeamSphereR() const;//右のビーム

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

	//effekseerの再生中のエフェクトのハンドル
	int m_effectPlayHandle = -1;

	//ステート
	std::shared_ptr<IBossEnemyState> m_pState;
	//敵生産工場
	std::weak_ptr<EnemyFactory> m_pEnemyFactory;
	//音のマネージャー
	std::weak_ptr<SoundManager> m_pSoundManager;

	//プレイヤーの弾が当たった時に、無敵判定する部分
	Sphere m_invincibleHitCol;
	//プレイヤーの弾が当たった時のダメージ判定する部分
	Sphere m_damageCol;

	//ボスが出現完了しているか
	//ゲームシーン側からセットさせる
	bool m_isAppear = false;
	//一番最初の着地時のみに使用するフラグ
	bool m_isFirstLanding = false;

	//シールドエフェクトの再生ハンドル
	int m_shieldEffectPlayH = -1;

	//死亡エフェクト再生ハンドル
	int m_deathEffectPlayH = -1;

	//死亡待機状態をどのぐらい続けるかを計測
	int m_dyingFrame = 0;

	//足音のクールタイム計測
	int m_footstepCT = 0;
};