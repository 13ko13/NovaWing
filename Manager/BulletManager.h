#pragma once
#include <vector>
#include <memory>
#include "Manager/ResourceLoader.h"
#include "Game/GameObjects/Bullet/ReflectedBullet.h"

class PlayerBullet;
class EnemyBullet;
class ChargeBullet;
class ReflectedBullet;
class GameObject;
struct Vector3;
class BulletBase;
class EnemyBase;
class CameraBase;
class EffectManager;
class BulletManager
{
public:
	BulletManager(std::weak_ptr<EffectManager> pEffectManager);
	~BulletManager();

	//更新処理
	void Update();

	//弾の種類
	enum class BulletType
	{
		PlayerBullet,//プレイヤーの弾
		ChargeBullet,//プレイヤーのチャージ弾
		EnemyBullet,//敵の弾
	};

	//別クラスから種類を指定してもらってその弾を作成する
	//ターゲットはデフォルトではNull,チャージショットの時はホーミング先、
	//敵弾の時は発射元の敵として使う
	void CreateBullet(const BulletType bulletType, const Vector3& pos,
	const Vector3& vel, const int attackPower, std::weak_ptr<CameraBase> pCamera,
		std::weak_ptr<EnemyBase> pTarget = std::weak_ptr<EnemyBase>());

	//他の弾には必要ない情報がいくつかあるので
	//反射弾の作成だけは別で行う
	void CreateReflectedBullet(
		const ReflectedBullet::ReflectBulletData& data
	);

	//プレイヤー弾の配列のゲッター
	const std::vector<std::weak_ptr<PlayerBullet>>& GetPlayerBullets() const 
	{ return m_pPlayerBullets; }
	//チャージ弾の配列のゲッター
	const std::vector<std::weak_ptr<ChargeBullet>>& GetChargeBullets() const
	{ return m_pChargeBullets; }
	//敵弾の配列のゲッター
	const std::vector<std::weak_ptr<EnemyBullet>>& GetEnemyBullets() const 
	{ return m_pEnemyBullets; }
	//反射弾の配列のゲッター
	const std::vector<std::weak_ptr<ReflectedBullet>>& GetReflectedBullets() const
	{ return m_pReflectedBullets; }

private:
	//敵弾の配列
	std::vector<std::weak_ptr<EnemyBullet>> m_pEnemyBullets;
	//反射弾の配列のゲッター
	std::vector<std::weak_ptr<ReflectedBullet>> m_pReflectedBullets;
	//プレイヤーの弾の配列
	std::vector<std::weak_ptr<PlayerBullet>> m_pPlayerBullets;
	//チャージ弾の配列
	std::vector<std::weak_ptr<ChargeBullet>> m_pChargeBullets;

	//弾全ての配列
	std::vector<std::weak_ptr<BulletBase>> m_pAllBullets;

	//弾が使うエフェクトのマネージャー
	std::weak_ptr<EffectManager> m_pEffectManager;
};