#pragma once
#include "Game/GameObjects/Actors/Charactor/Enemy/EnemyBase.h"
#include "Manager/ResourceLoader.h"
#include "Utility/Sphere.h"
#include "Utility/ModelAnimator.h"

class Player;
class BulletManager;
class IBossEnemyState;
class EnemyFactory;
class BossEnemy : public EnemyBase
{
public:
	//ボス生成に必要な情報
	struct BossEnemyData
	{
		std::weak_ptr<Player> pPlayer;
		ResourceLoader::ModelID Id;
		std::weak_ptr<BulletManager> pBulletManager;
		std::weak_ptr<CameraBase> pCamera;
		Vector3 pos;
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

	//自分の当たり判定を返す
	Sphere GetSphere() const { return m_hitCol; }

	//ビームを出しているときにビームの当たり判定を返す
	std::vector<Sphere> GetBeamSphereL() const;//左のビーム
	std::vector<Sphere> GetBeamSphereR() const;//右のビーム

	//現在のステートを返す
	std::shared_ptr<IBossEnemyState> GetCurrentState() const { return m_pState; }

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

	//プレイヤーの弾との当たり判定用球
	Sphere m_hitCol;
};